// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/backend/vulkan/context_vk.h"
#include <thread>
#include <unordered_map>

#include "core/formats.h"
#include "core/runtime_types.h"
#include "fml/concurrent_message_loop.h"
#include "renderer/backend/vulkan/command_queue_vk.h"
#include "renderer/backend/vulkan/descriptor_pool_vk.h"
#include "renderer/backend/vulkan/pipeline_library_vk.h"
#include "renderer/backend/vulkan/render_pass_builder_vk.h"
#include "renderer/backend/vulkan/workarounds_vk.h"
#include "renderer/render_target.h"

#ifdef FML_OS_ANDROID
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif  // FML_OS_ANDROID

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fml/cpu_affinity.h"
#include "fml/trace_event.h"
#include "renderer/backend/vulkan/allocator_vk.h"
#include "renderer/backend/vulkan/capabilities_vk.h"
#include "renderer/backend/vulkan/command_buffer_vk.h"
#include "renderer/backend/vulkan/command_pool_vk.h"
#include "renderer/backend/vulkan/debug_report_vk.h"
#include "renderer/backend/vulkan/descriptor_pool_vk.h"
#include "renderer/backend/vulkan/fence_waiter_vk.h"
#include "renderer/backend/vulkan/gpu_tracer_vk.h"
#include "renderer/backend/vulkan/resource_manager_vk.h"
#include "renderer/backend/vulkan/tracked_objects_vk.h"
#include "renderer/backend/vulkan/yuv_conversion_library_vk.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace ogre {

static bool gHasValidationLayers = false;

bool HasValidationLayers() {
  return gHasValidationLayers;
}

static std::optional<vk::PhysicalDevice> PickPhysicalDevice(
    const Capabilities& caps,
    const vk::Instance& instance) {
  for (const auto& device : instance.enumeratePhysicalDevices().value) {
    if (caps.GetEnabledDeviceFeatures(device).has_value()) {
      return device;
    }
  }
  return std::nullopt;
}

static std::vector<vk::DeviceQueueCreateInfo> GetQueueCreateInfos(
    std::initializer_list<QueueIndex> queues) {
  std::map<size_t /* family */, size_t /* index */> family_index_map;
  for (const auto& queue : queues) {
    family_index_map[queue.family] = 0;
  }
  for (const auto& queue : queues) {
    auto value = family_index_map[queue.family];
    family_index_map[queue.family] = std::max(value, queue.index);
  }

  static float kQueuePriority = 1.0f;
  std::vector<vk::DeviceQueueCreateInfo> infos;
  for (const auto& item : family_index_map) {
    vk::DeviceQueueCreateInfo info;
    info.setQueueFamilyIndex(item.first);
    info.setQueueCount(item.second + 1);
    info.setQueuePriorities(kQueuePriority);
    infos.push_back(info);
  }
  return infos;
}

static std::optional<QueueIndex> PickQueue(const vk::PhysicalDevice& device,
                                           vk::QueueFlagBits flags) {
  // This can be modified to ensure that dedicated queues are returned for each
  // queue type depending on support.
  const auto families = device.getQueueFamilyProperties();
  for (size_t i = 0u; i < families.size(); i++) {
    if (!(families[i].queueFlags & flags)) {
      continue;
    }
    return QueueIndex{.family = i, .index = 0};
  }
  return std::nullopt;
}

std::shared_ptr<Context> Context::Create(Settings settings) {
  auto context = std::shared_ptr<Context>(new Context());
  context->Setup(std::move(settings));
  if (!context->IsValid()) {
    return nullptr;
  }
  return context;
}

// static
size_t Context::ChooseThreadCountForWorkers(size_t hardware_concurrency) {
  // Never create more than 4 worker threads. Attempt to use up to
  // half of the available concurrency.
  return std::clamp(hardware_concurrency / 2ull, /*lo=*/1ull, /*hi=*/4ull);
}

namespace {
std::atomic_uint64_t context_count = 0;
uint64_t CalculateHash(void* ptr) {
  return context_count.fetch_add(1);
}
}  // namespace

Context::Context() : hash_(CalculateHash(this)) {}

Context::~Context() {
  if (device_holder_ && device_holder_->device) {
    [[maybe_unused]] auto result = device_holder_->device->waitIdle();
  }
  if (command_pool_recycler_) {
    command_pool_recycler_->DestroyThreadLocalPools();
  }
}

Context::BackendType Context::GetBackendType() const {
  return Context::BackendType::kVulkan;
}

/* version 2.0.0 */
static constexpr uint32_t kImpellerEngineVersion =
    VK_MAKE_API_VERSION(0, 2, 0, 0);

void Context::Setup(Settings settings) {
  TRACE_EVENT0("ogre", "ContextVK::Setup");

  if (!settings.proc_address_callback) {
    LOG(ERROR) << "Missing proc address callback.";
    return;
  }

  raster_message_loop_ = fml::ConcurrentMessageLoop::Create(
      ChooseThreadCountForWorkers(std::thread::hardware_concurrency()));

  auto& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
  dispatcher.init(settings.proc_address_callback);

  std::vector<std::string> embedder_instance_extensions;
  std::vector<std::string> embedder_device_extensions;
  if (settings.embedder_data.has_value()) {
    embedder_instance_extensions = settings.embedder_data->instance_extensions;
    embedder_device_extensions = settings.embedder_data->device_extensions;
  }
  auto caps = std::shared_ptr<Capabilities>(new Capabilities(
      settings.enable_validation,                                      //
      settings.fatal_missing_validations,                              //
      /*use_embedder_extensions=*/settings.embedder_data.has_value(),  //
      embedder_instance_extensions,                                    //
      embedder_device_extensions                                       //
      ));

  if (!caps->IsValid()) {
    LOG(ERROR) << "Could not determine device capabilities.";
    return;
  }

  gHasValidationLayers = caps->AreValidationsEnabled();

  auto enabled_layers = caps->GetEnabledLayers();
  auto enabled_extensions = caps->GetEnabledInstanceExtensions();

  if (!enabled_layers.has_value() || !enabled_extensions.has_value()) {
    LOG(ERROR) << "Device has insufficient capabilities.";
    return;
  }

  vk::InstanceCreateFlags instance_flags = {};

  if (std::find(enabled_extensions.value().begin(),
                enabled_extensions.value().end(),
                "VK_KHR_portability_enumeration") !=
      enabled_extensions.value().end()) {
    instance_flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  }

  std::vector<const char*> enabled_layers_c;
  std::vector<const char*> enabled_extensions_c;

  for (const auto& layer : enabled_layers.value()) {
    enabled_layers_c.push_back(layer.c_str());
  }

  for (const auto& ext : enabled_extensions.value()) {
    enabled_extensions_c.push_back(ext.c_str());
  }

  vk::ApplicationInfo application_info;

  // Use the same encoding macro as vulkan versions, but otherwise engine
  // version is intended to be the version of the Impeller engine. This version
  // information, along with the application name below is provided to allow
  // IHVs to make optimizations and/or disable functionality based on knowledge
  // of the engine version (for example, to work around bugs). We don't tie this
  // to the overall Flutter version as that version is not yet defined when the
  // engine is compiled. Instead we can manually bump it occasionally.
  //
  // variant, major, minor, patch
  application_info.setApplicationVersion(VK_API_VERSION_1_0);
  application_info.setApiVersion(VK_API_VERSION_1_1);
  application_info.setEngineVersion(kImpellerEngineVersion);
  application_info.setPEngineName("Impeller");
  application_info.setPApplicationName("Impeller");

  vk::StructureChain<vk::InstanceCreateInfo, vk::ValidationFeaturesEXT>
      instance_chain;

  if (!caps->AreValidationsEnabled()) {
    instance_chain.unlink<vk::ValidationFeaturesEXT>();
  }

  std::vector<vk::ValidationFeatureEnableEXT> enabled_validations = {
      vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
  };

  auto validation = instance_chain.get<vk::ValidationFeaturesEXT>();
  validation.setEnabledValidationFeatures(enabled_validations);

  auto instance_info = instance_chain.get<vk::InstanceCreateInfo>();
  instance_info.setPEnabledLayerNames(enabled_layers_c);
  instance_info.setPEnabledExtensionNames(enabled_extensions_c);
  instance_info.setPApplicationInfo(&application_info);
  instance_info.setFlags(instance_flags);

  auto device_holder = std::make_shared<DeviceHolderImpl>();
  if (!settings.embedder_data.has_value()) {
    auto instance = vk::createInstanceUnique(instance_info);
    if (instance.result != vk::Result::eSuccess) {
      LOG(ERROR) << "Could not create Vulkan instance: "
                 << vk::to_string(instance.result);
      return;
    }
    device_holder->instance = std::move(instance.value);
  } else {
    device_holder->instance.reset(settings.embedder_data->instance);
    device_holder->owned = false;
  }
  dispatcher.init(device_holder->instance.get());

  //----------------------------------------------------------------------------
  /// Setup the debug report.
  ///
  /// Do this as early as possible since we could use the debug report from
  /// initialization issues.
  ///
  auto debug_report =
      std::make_unique<DebugReport>(*caps, device_holder->instance.get());

  if (!debug_report->IsValid()) {
    LOG(ERROR) << "Could not set up debug report.";
    return;
  }

  //----------------------------------------------------------------------------
  /// Pick the physical device.
  ///
  if (!settings.embedder_data.has_value()) {
    auto physical_device =
        PickPhysicalDevice(*caps, device_holder->instance.get());
    if (!physical_device.has_value()) {
      LOG(ERROR) << "No valid Vulkan device found.";
      return;
    }
    device_holder->physical_device = physical_device.value();
  } else {
    device_holder->physical_device = settings.embedder_data->physical_device;
  }

  //----------------------------------------------------------------------------
  /// Pick device queues.
  ///
  auto graphics_queue =
      PickQueue(device_holder->physical_device, vk::QueueFlagBits::eGraphics);
  auto transfer_queue =
      PickQueue(device_holder->physical_device, vk::QueueFlagBits::eTransfer);
  auto compute_queue =
      PickQueue(device_holder->physical_device, vk::QueueFlagBits::eCompute);

  if (!graphics_queue.has_value()) {
    LOG(ERROR) << "Could not pick graphics queue.";
    return;
  }
  if (!transfer_queue.has_value()) {
    transfer_queue = graphics_queue.value();
  }
  if (!compute_queue.has_value()) {
    LOG(ERROR) << "Could not pick compute queue.";
    return;
  }

  //----------------------------------------------------------------------------
  /// Create the logical device.
  ///
  auto enabled_device_extensions =
      caps->GetEnabledDeviceExtensions(device_holder->physical_device);
  if (!enabled_device_extensions.has_value()) {
    // This shouldn't happen since we already did device selection. But
    // doesn't hurt to check again.
    return;
  }

  std::vector<const char*> enabled_device_extensions_c;
  for (const auto& ext : enabled_device_extensions.value()) {
    enabled_device_extensions_c.push_back(ext.c_str());
  }

  const auto queue_create_infos = GetQueueCreateInfos(
      {graphics_queue.value(), compute_queue.value(), transfer_queue.value()});

  const auto enabled_features =
      caps->GetEnabledDeviceFeatures(device_holder->physical_device);
  if (!enabled_features.has_value()) {
    // This shouldn't happen since the device can't be picked if this was not
    // true. But doesn't hurt to check.
    return;
  }

  vk::DeviceCreateInfo device_info;

  device_info.setPNext(&enabled_features.value().get());
  device_info.setQueueCreateInfos(queue_create_infos);
  device_info.setPEnabledExtensionNames(enabled_device_extensions_c);
  // Device layers are deprecated and ignored.

  if (!settings.embedder_data.has_value()) {
    auto device_result =
        device_holder->physical_device.createDeviceUnique(device_info);
    if (device_result.result != vk::Result::eSuccess) {
      LOG(ERROR) << "Could not create logical device.";
      return;
    }
    device_holder->device = std::move(device_result.value);
  } else {
    device_holder->device.reset(settings.embedder_data->device);
  }

  if (!caps->SetPhysicalDevice(device_holder->physical_device,
                               *enabled_features)) {
    LOG(ERROR) << "Capabilities could not be updated.";
    return;
  }

  //----------------------------------------------------------------------------
  /// Create the allocator.
  ///
  auto allocator = std::shared_ptr<Allocator>(new Allocator(
      weak_from_this(),                //
      application_info.apiVersion,     //
      device_holder->physical_device,  //
      device_holder,                   //
      device_holder->instance.get(),   //
      *caps                            //
      ));

  if (!allocator->IsValid()) {
    LOG(ERROR) << "Could not create memory allocator.";
    return;
  }

  //----------------------------------------------------------------------------
  /// Setup the pipeline library.
  ///
  auto pipeline_library = std::shared_ptr<PipelineLibrary>(new PipelineLibrary(
      device_holder,                         //
      caps,                                  //
      std::move(settings.cache_directory),   //
      raster_message_loop_->GetTaskRunner()  //
      ));

  if (!pipeline_library->IsValid()) {
    LOG(ERROR) << "Could not create pipeline library.";
    return;
  }

  auto sampler_library =
      std::shared_ptr<SamplerLibrary>(new SamplerLibrary(device_holder));

  auto shader_library = std::shared_ptr<ShaderLibrary>(
      new ShaderLibrary(device_holder,                   //
                        settings.shader_libraries_data)  //
  );

  // if (!shader_library->IsValid()) {
  //   LOG(ERROR) << "Could not create shader library.";
  //   return;
  // }

  //----------------------------------------------------------------------------
  /// Create the fence waiter.
  ///
  auto fence_waiter =
      std::shared_ptr<FenceWaiter>(new FenceWaiter(device_holder));

  //----------------------------------------------------------------------------
  /// Create the resource manager and command pool recycler.
  ///
  auto resource_manager = ResourceManager::Create();
  if (!resource_manager) {
    LOG(ERROR) << "Could not create resource manager.";
    return;
  }

  auto command_pool_recycler =
      std::make_shared<CommandPoolRecycler>(shared_from_this());
  if (!command_pool_recycler) {
    LOG(ERROR) << "Could not create command pool recycler.";
    return;
  }

  auto descriptor_pool_recycler =
      std::make_shared<DescriptorPoolRecycler>(weak_from_this());
  if (!descriptor_pool_recycler) {
    LOG(ERROR) << "Could not create descriptor pool recycler.";
    return;
  }

  //----------------------------------------------------------------------------
  /// Fetch the queues.
  ///
  QueuesVK queues;
  if (!settings.embedder_data.has_value()) {
    queues = QueuesVK::FromQueueIndices(device_holder->device.get(),  //
                                        graphics_queue.value(),       //
                                        compute_queue.value(),        //
                                        transfer_queue.value()        //
    );
  } else {
    queues =
        QueuesVK::FromEmbedderQueue(settings.embedder_data->queue,
                                    settings.embedder_data->queue_family_index);
  }
  if (!queues.IsValid()) {
    LOG(ERROR) << "Could not fetch device queues.";
    return;
  }

  VkPhysicalDeviceProperties physical_device_properties;
  dispatcher.vkGetPhysicalDeviceProperties(device_holder->physical_device,
                                           &physical_device_properties);

  //----------------------------------------------------------------------------
  /// All done!
  ///

  // Apply workarounds for broken drivers.
  auto driver_info =
      std::make_unique<DriverInfo>(device_holder->physical_device);
  workarounds_ = GetWorkaroundsFromDriverInfo(*driver_info);
  caps->ApplyWorkarounds(workarounds_);
  sampler_library->ApplyWorkarounds(workarounds_);

  device_holder_ = std::move(device_holder);
  idle_waiter_vk_ = std::make_shared<IdleWaiter>(device_holder_);
  driver_info_ = std::move(driver_info);
  debug_report_ = std::move(debug_report);
  allocator_ = std::move(allocator);
  shader_library_ = std::move(shader_library);
  sampler_library_ = std::move(sampler_library);
  pipeline_library_ = std::move(pipeline_library);
  yuv_conversion_library_ = std::shared_ptr<YUVConversionLibrary>(
      new YUVConversionLibrary(device_holder_));
  queues_ = std::move(queues);
  device_capabilities_ = std::move(caps);
  fence_waiter_ = std::move(fence_waiter);
  resource_manager_ = std::move(resource_manager);
  command_pool_recycler_ = std::move(command_pool_recycler);
  descriptor_pool_recycler_ = std::move(descriptor_pool_recycler);
  device_name_ = std::string(physical_device_properties.deviceName);
  command_queue_vk_ = std::make_shared<CommandQueue>(weak_from_this());
  should_enable_surface_control_ = settings.enable_surface_control;
  should_batch_cmd_buffers_ = !workarounds_.batch_submit_command_buffer_timeout;
  is_valid_ = true;

  // Create the GPU Tracer later because it depends on state from
  // the ContextVK.
  gpu_tracer_ = std::make_shared<GPUTracer>(weak_from_this(),
                                            settings.enable_gpu_tracing);
  gpu_tracer_->InitializeQueryPool(*this);

  //----------------------------------------------------------------------------
  /// Label all the relevant objects. This happens after setup so that the
  /// debug messengers have had a chance to be set up.
  ///
  SetDebugName(GetDevice(), device_holder_->device.get(), "ImpellerDevice");
}

void Context::SetOffscreenFormat(PixelFormat pixel_format) {
  const_cast<Capabilities&>(*device_capabilities_)
      .SetOffscreenFormat(pixel_format);
}

// |Context|
std::string Context::DescribeGpuModel() const {
  return device_name_;
}

bool Context::IsValid() const {
  return is_valid_;
}

std::shared_ptr<Allocator> Context::GetResourceAllocator() const {
  return allocator_;
}

std::shared_ptr<ShaderLibrary> Context::GetShaderLibrary() const {
  return shader_library_;
}

std::shared_ptr<SamplerLibrary> Context::GetSamplerLibrary() const {
  return sampler_library_;
}

std::shared_ptr<PipelineLibrary> Context::GetPipelineLibrary() const {
  return pipeline_library_;
}

std::shared_ptr<CommandBuffer> Context::CreateCommandBuffer() const {
  const auto& recycler = GetCommandPoolRecycler();
  auto tls_pool = recycler->Get();
  if (!tls_pool) {
    return nullptr;
  }

  // look up a cached descriptor pool for the current frame and reuse it
  // if it exists, otherwise create a new pool.
  std::shared_ptr<DescriptorPool> descriptor_pool;
  {
    Lock lock(desc_pool_mutex_);
    DescriptorPoolMap::iterator current_pool =
        cached_descriptor_pool_.find(std::this_thread::get_id());
    if (current_pool == cached_descriptor_pool_.end()) {
      descriptor_pool = (cached_descriptor_pool_[std::this_thread::get_id()] =
                             descriptor_pool_recycler_->GetDescriptorPool());
    } else {
      descriptor_pool = current_pool->second;
    }
  }

  auto tracked_objects = std::make_shared<TrackedObjects>(
      weak_from_this(), std::move(tls_pool), std::move(descriptor_pool),
      GetGPUTracer()->CreateGPUProbe());
  auto queue = GetGraphicsQueue();

  if (!tracked_objects || !tracked_objects->IsValid() || !queue) {
    return nullptr;
  }

  vk::CommandBufferBeginInfo begin_info;
  begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  if (tracked_objects->GetCommandBuffer().begin(begin_info) !=
      vk::Result::eSuccess) {
    LOG(ERROR) << "Could not begin command buffer.";
    return nullptr;
  }

  tracked_objects->GetGPUProbe().RecordCmdBufferStart(
      tracked_objects->GetCommandBuffer());

  return std::shared_ptr<CommandBuffer>(new CommandBuffer(
      shared_from_this(),         //
      GetDeviceHolder(),          //
      std::move(tracked_objects)  //
      ));
}

vk::Instance Context::GetInstance() const {
  return *device_holder_->instance;
}

const vk::Device& Context::GetDevice() const {
  return device_holder_->device.get();
}

const std::shared_ptr<fml::ConcurrentTaskRunner>
Context::GetConcurrentWorkerTaskRunner() const {
  return raster_message_loop_->GetTaskRunner();
}

void Context::Shutdown() {
  // There are multiple objects, for example |CommandPool|, that in their
  // destructors make a strong reference to |ContextVK|. Resetting these shared
  // pointers ensures that cleanup happens in a correct order.
  //
  // tl;dr: Without it, we get thread::join failures on shutdown.
  fence_waiter_->Terminate();
  resource_manager_.reset();

  raster_message_loop_->Terminate();
}

const std::shared_ptr<const Capabilities>& Context::GetCapabilities() const {
  return device_capabilities_;
}

const std::shared_ptr<Queue>& Context::GetGraphicsQueue() const {
  return queues_.graphics_queue;
}

vk::PhysicalDevice Context::GetPhysicalDevice() const {
  return device_holder_->physical_device;
}

std::shared_ptr<FenceWaiter> Context::GetFenceWaiter() const {
  return fence_waiter_;
}

std::shared_ptr<ResourceManager> Context::GetResourceManager() const {
  return resource_manager_;
}

std::shared_ptr<CommandPoolRecycler> Context::GetCommandPoolRecycler() const {
  return command_pool_recycler_;
}

std::shared_ptr<GPUTracer> Context::GetGPUTracer() const {
  return gpu_tracer_;
}

std::shared_ptr<DescriptorPoolRecycler> Context::GetDescriptorPoolRecycler()
    const {
  return descriptor_pool_recycler_;
}

std::shared_ptr<CommandQueue> Context::GetCommandQueue() const {
  return command_queue_vk_;
}

bool Context::EnqueueCommandBuffer(
    std::shared_ptr<CommandBuffer> command_buffer) {
  if (should_batch_cmd_buffers_) {
    pending_command_buffers_.push_back(std::move(command_buffer));
    return true;
  } else {
    return GetCommandQueue()->Submit({command_buffer}).ok();
  }
}

bool Context::FlushCommandBuffers() {
  if (pending_command_buffers_.empty()) {
    return true;
  }

  if (should_batch_cmd_buffers_) {
    bool result = GetCommandQueue()->Submit(pending_command_buffers_).ok();
    pending_command_buffers_.clear();
    return result;
  } else {
    return true;
  }
}

// Creating a render pass is observed to take an additional 6ms on a Pixel 7
// device as the driver will lazily bootstrap and compile shaders to do so.
// The render pass does not need to be begun or executed.
void Context::InitializeCommonlyUsedShadersIfNeeded() const {
  RenderTargetAllocator rt_allocator(GetResourceAllocator());
  RenderTarget render_target =
      rt_allocator.CreateOffscreenMSAA(*this, {1, 1}, 1);

  RenderPassBuilder builder;

  render_target.IterateAllColorAttachments(
      [&builder](size_t index, const ColorAttachment& attachment) -> bool {
        builder.SetColorAttachment(
            index,                                                    //
            attachment.texture->GetTextureDescriptor().format,        //
            attachment.texture->GetTextureDescriptor().sample_count,  //
            attachment.load_action,                                   //
            attachment.store_action                                   //
        );
        return true;
      });

  if (const auto& depth = render_target.GetDepthAttachment();
      depth.has_value()) {
    builder.SetDepthStencilAttachment(
        depth->texture->GetTextureDescriptor().format,        //
        depth->texture->GetTextureDescriptor().sample_count,  //
        depth->load_action,                                   //
        depth->store_action                                   //
    );
  } else if (const auto& stencil = render_target.GetStencilAttachment();
             stencil.has_value()) {
    builder.SetStencilAttachment(
        stencil->texture->GetTextureDescriptor().format,        //
        stencil->texture->GetTextureDescriptor().sample_count,  //
        stencil->load_action,                                   //
        stencil->store_action                                   //
    );
  }

  auto pass = builder.Build(GetDevice());
}

void Context::DisposeThreadLocalCachedResources() {
  {
    Lock lock(desc_pool_mutex_);
    cached_descriptor_pool_.erase(std::this_thread::get_id());
  }
  command_pool_recycler_->Dispose();
}

const std::shared_ptr<YUVConversionLibrary>& Context::GetYUVConversionLibrary()
    const {
  return yuv_conversion_library_;
}

const std::unique_ptr<DriverInfo>& Context::GetDriverInfo() const {
  return driver_info_;
}

bool Context::GetShouldEnableSurfaceControlSwapchain() const {
  return should_enable_surface_control_ &&
         device_capabilities_->SupportsExternalSemaphoreExtensions();
}

RuntimeStageBackend Context::GetRuntimeStageBackend() const {
  return RuntimeStageBackend::kVulkan;
}

bool Context::SubmitOnscreen(std::shared_ptr<CommandBuffer> cmd_buffer) {
  return EnqueueCommandBuffer(std::move(cmd_buffer));
}

const Workarounds& Context::GetWorkarounds() const {
  return workarounds_;
}

bool Context::UpdateOffscreenLayerPixelFormat(PixelFormat format) {
  return false;
}

bool Context::AddTrackingFence(const std::shared_ptr<Texture>& texture) const {
  return false;
}

void Context::ResetThreadLocalState() const {
  // Nothing to do.
}

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <format>
#include <memory>

#include <absl/log/check.h>
#include "base/flags.h"
#include "base/thread_safety.h"
#include "base/validation.h"
#include "core/formats.h"
#include "core/runtime_types.h"
#include "fml/closure.h"
#include "fml/concurrent_message_loop.h"
#include "fml/mapping.h"
#include "fml/unique_fd.h"
#include "renderer/backend/vulkan/capabilities_vk.h"
#include "renderer/backend/vulkan/command_pool_vk.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/driver_info_vk.h"
#include "renderer/backend/vulkan/idle_waiter_vk.h"
#include "renderer/backend/vulkan/queue_vk.h"
#include "renderer/backend/vulkan/sampler_library_vk.h"
#include "renderer/backend/vulkan/shader_library_vk.h"
#include "renderer/backend/vulkan/workarounds_vk.h"

namespace ogre {

bool HasValidationLayers();

class Allocator;
class Capabilities;
class CommandBuffer;
class CommandEncoder;
class CommandPool;
class CommandPoolRecycler;
class CommandQueue;
class DebugReport;
class DescriptorPool;
class DescriptorPoolRecycler;
class FenceWaiter;
class GPUTracer;
class IdleWaiter;
class PipelineLibrary;
class ResourceManager;
class SamplerLibrary;
class ShaderLibrary;
class Texture;
class YUVConversionLibrary;

class Context final : public std::enable_shared_from_this<Context> {
 public:
  enum class BackendType {
    kMetal,
    kOpenGLES,
    kVulkan,
  };

  /// The maximum number of tasks that should ever be stored for
  /// `StoreTaskForGPU`.
  static constexpr int32_t kMaxTasksAwaitingGPU = 1024;

  /// Embedder Stuff
  struct EmbedderData {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t queue_family_index;
    VkQueue queue;
    std::vector<std::string> instance_extensions;
    std::vector<std::string> device_extensions;
  };

  struct Settings {
    PFN_vkGetInstanceProcAddr proc_address_callback = nullptr;
    std::vector<std::shared_ptr<fml::Mapping>> shader_libraries_data;
    fml::UniqueFD cache_directory;
    bool enable_validation = false;
    bool enable_gpu_tracing = false;
    bool enable_surface_control = false;
    /// If validations are requested but cannot be enabled, log a fatal error.
    bool fatal_missing_validations = false;
    Flags flags;

    std::optional<EmbedderData> embedder_data;

    Settings() = default;

    Settings(Settings&&) = default;
  };

  /// Choose the number of worker threads the context_vk will create.
  ///
  /// Visible for testing.
  static size_t ChooseThreadCountForWorkers(size_t hardware_concurrency);

  static std::shared_ptr<Context> Create(Settings settings);

  uint64_t GetHash() const { return hash_; }

  ~Context();

  BackendType GetBackendType() const;

  std::string DescribeGpuModel() const;

  bool IsValid() const;

  std::shared_ptr<Allocator> GetResourceAllocator() const;

  std::shared_ptr<ShaderLibrary> GetShaderLibrary() const;

  std::shared_ptr<SamplerLibrary> GetSamplerLibrary() const;

  std::shared_ptr<PipelineLibrary> GetPipelineLibrary() const;

  std::shared_ptr<CommandBuffer> CreateCommandBuffer() const;

  const std::shared_ptr<const Capabilities>& GetCapabilities() const;

  bool UpdateOffscreenLayerPixelFormat(PixelFormat format);

  bool SubmitOnscreen(std::shared_ptr<CommandBuffer> cmd_buffer);

  const std::shared_ptr<YUVConversionLibrary>& GetYUVConversionLibrary() const;

  void Shutdown();

  const Workarounds& GetWorkarounds() const;

  void SetOffscreenFormat(PixelFormat pixel_format);

  template <typename T>
  bool SetDebugName(T handle, std::string_view label) const {
    return SetDebugName(GetDevice(), handle, label);
  }

  template <typename T>
  bool SetDebugName(T handle,
                    std::string_view label,
                    std::string_view trailing) const {
    if (!HasValidationLayers()) {
      // No-op if validation layers are not enabled.
      return true;
    }
    std::string combined = std::format("{} {}", label, trailing);
    return SetDebugName(GetDevice(), handle, combined);
  }

  template <typename T>
  static bool SetDebugName(const vk::Device& device,
                           T handle,
                           std::string_view label) {
    if (!HasValidationLayers()) {
      // No-op if validation layers are not enabled.
      return true;
    }

    auto c_handle = static_cast<typename T::CType>(handle);

    vk::DebugUtilsObjectNameInfoEXT info;
    info.objectType = T::objectType;
    info.pObjectName = label.data();
    info.objectHandle = reinterpret_cast<decltype(info.objectHandle)>(c_handle);

    if (device.setDebugUtilsObjectNameEXT(info) != vk::Result::eSuccess) {
      VALIDATION_LOG << "Unable to set debug name: " << label;
      return false;
    }

    return true;
  }

  std::shared_ptr<DeviceHolder> GetDeviceHolder() const {
    return device_holder_;
  }

  vk::Instance GetInstance() const;

  const vk::Device& GetDevice() const;

  const std::unique_ptr<DriverInfo>& GetDriverInfo() const;

  const std::shared_ptr<fml::ConcurrentTaskRunner>
  GetConcurrentWorkerTaskRunner() const;

  const std::shared_ptr<Queue>& GetGraphicsQueue() const;

  vk::PhysicalDevice GetPhysicalDevice() const;

  std::shared_ptr<FenceWaiter> GetFenceWaiter() const;

  std::shared_ptr<ResourceManager> GetResourceManager() const;

  std::shared_ptr<CommandPoolRecycler> GetCommandPoolRecycler() const;

  std::shared_ptr<DescriptorPoolRecycler> GetDescriptorPoolRecycler() const;

  std::shared_ptr<CommandQueue> GetCommandQueue() const;

  std::shared_ptr<GPUTracer> GetGPUTracer() const;

  void RecordFrameEndTime() const;

  void InitializeCommonlyUsedShadersIfNeeded() const;

  void DisposeThreadLocalCachedResources();

  /// @brief Whether the Android Surface control based swapchain should be
  ///        enabled
  bool GetShouldEnableSurfaceControlSwapchain() const;

  bool EnqueueCommandBuffer(std::shared_ptr<CommandBuffer> command_buffer);

  bool FlushCommandBuffers();

  RuntimeStageBackend GetRuntimeStageBackend() const;

  std::shared_ptr<const IdleWaiter> GetIdleWaiter() const {
    return idle_waiter_vk_;
  }

  void StoreTaskForGPU(const fml::closure& task, const fml::closure& failure) {
    CHECK(false && "not supported in this context");
  }

  bool AddTrackingFence(const std::shared_ptr<Texture>& texture) const;

  void ResetThreadLocalState() const;

  const Flags& GetFlags() const { return flags_; }

 private:
  struct DeviceHolderImpl : public DeviceHolder {
    // |DeviceHolder|
    const vk::Device& GetDevice() const override { return device.get(); }
    // |DeviceHolder|
    const vk::PhysicalDevice& GetPhysicalDevice() const override {
      return physical_device;
    }

    ~DeviceHolderImpl() {
      if (!owned) {
        instance.release();
        device.release();
      }
    }

    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    bool owned = true;
  };

  std::shared_ptr<DeviceHolderImpl> device_holder_;
  std::unique_ptr<DriverInfo> driver_info_;
  std::unique_ptr<DebugReport> debug_report_;
  std::shared_ptr<Allocator> allocator_;
  std::shared_ptr<ShaderLibrary> shader_library_;
  std::shared_ptr<SamplerLibrary> sampler_library_;
  std::shared_ptr<PipelineLibrary> pipeline_library_;
  std::shared_ptr<YUVConversionLibrary> yuv_conversion_library_;
  QueuesVK queues_;
  std::shared_ptr<const Capabilities> device_capabilities_;
  std::shared_ptr<FenceWaiter> fence_waiter_;
  std::shared_ptr<ResourceManager> resource_manager_;
  std::shared_ptr<DescriptorPoolRecycler> descriptor_pool_recycler_;
  std::shared_ptr<CommandPoolRecycler> command_pool_recycler_;
  std::string device_name_;
  std::shared_ptr<fml::ConcurrentMessageLoop> raster_message_loop_;
  std::shared_ptr<GPUTracer> gpu_tracer_;
  std::shared_ptr<CommandQueue> command_queue_vk_;
  std::shared_ptr<const IdleWaiter> idle_waiter_vk_;
  Workarounds workarounds_;
  Flags flags_;
  std::vector<std::function<void()>> per_frame_task_;

  using DescriptorPoolMap =
      std::unordered_map<std::thread::id, std::shared_ptr<DescriptorPool>>;

  mutable Mutex desc_pool_mutex_;
  mutable DescriptorPoolMap IPLR_GUARDED_BY(desc_pool_mutex_)
      cached_descriptor_pool_;
  bool should_enable_surface_control_ = false;
  bool should_batch_cmd_buffers_ = false;
  std::vector<std::shared_ptr<CommandBuffer>> pending_command_buffers_;

  const uint64_t hash_;

  bool is_valid_ = false;

  explicit Context(const Flags& flags);

  void Setup(Settings settings);

  Context(const Context&) = delete;

  Context& operator=(const Context&) = delete;
};

}  // namespace ogre

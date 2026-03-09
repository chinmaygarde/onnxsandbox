// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/pipeline_library_vk.h"

#include <cstdint>

#include <absl/log/check.h>
#include "base/promise.h"

#include "fml/container.h"
#include "fml/trace_event.h"
#include "hal/context_vk.h"
#include "hal/formats_vk.h"
#include "hal/pipeline_vk.h"
#include "hal/shader_function_vk.h"

namespace ogre {

PipelineLibrary::PipelineLibrary(
    const std::shared_ptr<DeviceHolder>& device_holder,
    std::shared_ptr<const Capabilities> caps,
    fml::UniqueFD cache_directory,
    std::shared_ptr<fml::ConcurrentTaskRunner> worker_task_runner)
    : device_holder_(device_holder),
      pso_cache_(std::make_shared<PipelineCache>(std::move(caps),
                                                 device_holder,
                                                 std::move(cache_directory))),
      worker_task_runner_(std::move(worker_task_runner)),
      compile_queue_(PipelineCompileQueue::Create(worker_task_runner_)) {
  DCHECK(worker_task_runner_);
  if (!pso_cache_->IsValid() || !worker_task_runner_) {
    return;
  }

  is_valid_ = true;
}

PipelineLibrary::~PipelineLibrary() = default;

// |PipelineLibrary|
bool PipelineLibrary::IsValid() const {
  return is_valid_;
}

std::unique_ptr<ComputePipeline> PipelineLibrary::CreateComputePipeline(
    const ComputePipelineDescriptor& desc,
    PipelineKey pipeline_key) {
  TRACE_EVENT0("flutter", __FUNCTION__);
  vk::ComputePipelineCreateInfo pipeline_info;

  //----------------------------------------------------------------------------
  /// Shader Stage
  ///
  const auto entrypoint = desc.GetStageEntrypoint();
  if (!entrypoint) {
    LOG(ERROR) << "Compute shader is missing an entrypoint.";
    return nullptr;
  }

  std::shared_ptr<DeviceHolder> strong_device = device_holder_.lock();
  if (!strong_device) {
    return nullptr;
  }
  auto device_properties = strong_device->GetPhysicalDevice().getProperties();
  auto max_wg_size = device_properties.limits.maxComputeWorkGroupSize;

  // Give all compute shaders a specialization constant entry for the
  // workgroup/threadgroup size.
  vk::SpecializationMapEntry specialization_map_entry[1];

  uint32_t workgroup_size_x = max_wg_size[0];
  specialization_map_entry[0].constantID = 0;
  specialization_map_entry[0].offset = 0;
  specialization_map_entry[0].size = sizeof(uint32_t);

  vk::SpecializationInfo specialization_info;
  specialization_info.mapEntryCount = 1;
  specialization_info.pMapEntries = &specialization_map_entry[0];
  specialization_info.dataSize = sizeof(uint32_t);
  specialization_info.pData = &workgroup_size_x;

  vk::PipelineShaderStageCreateInfo info;
  info.setStage(vk::ShaderStageFlagBits::eCompute);
  info.setPName("main");
  info.setModule(entrypoint->GetModule());
  info.setPSpecializationInfo(&specialization_info);
  pipeline_info.setStage(info);

  //----------------------------------------------------------------------------
  /// Pipeline Layout a.k.a the descriptor sets and uniforms.
  ///
  std::vector<vk::DescriptorSetLayoutBinding> desc_bindings;

  for (auto layout : desc.GetDescriptorSetLayouts()) {
    auto vk_desc_layout = ToVKDescriptorSetLayoutBinding(layout);
    desc_bindings.push_back(vk_desc_layout);
  }

  vk::DescriptorSetLayoutCreateInfo descs_layout_info;
  descs_layout_info.setBindings(desc_bindings);

  auto [descs_result, descs_layout] =
      strong_device->GetDevice().createDescriptorSetLayoutUnique(
          descs_layout_info);
  if (descs_result != vk::Result::eSuccess) {
    LOG(ERROR) << "unable to create uniform descriptors";
    return nullptr;
  }

  Context::SetDebugName(strong_device->GetDevice(), descs_layout.get(),
                        "Descriptor Set Layout " + desc.GetLabel());

  //----------------------------------------------------------------------------
  /// Create the pipeline layout.
  ///
  vk::PipelineLayoutCreateInfo pipeline_layout_info;
  pipeline_layout_info.setSetLayouts(descs_layout.get());
  auto pipeline_layout = strong_device->GetDevice().createPipelineLayoutUnique(
      pipeline_layout_info);
  if (pipeline_layout.result != vk::Result::eSuccess) {
    LOG(ERROR) << "Could not create pipeline layout for pipeline "
               << desc.GetLabel() << ": "
               << vk::to_string(pipeline_layout.result);
    return nullptr;
  }
  pipeline_info.setLayout(pipeline_layout.value.get());

  //----------------------------------------------------------------------------
  /// Finally, all done with the setup info. Create the pipeline itself.
  ///
  auto pipeline = pso_cache_->CreatePipeline(pipeline_info);
  if (!pipeline) {
    LOG(ERROR) << "Could not create graphics pipeline: " << desc.GetLabel();
    return nullptr;
  }

  Context::SetDebugName(strong_device->GetDevice(), *pipeline_layout.value,
                        "Pipeline Layout " + desc.GetLabel());
  Context::SetDebugName(strong_device->GetDevice(), *pipeline,
                        "Pipeline " + desc.GetLabel());

  return std::make_unique<ComputePipeline>(device_holder_,
                                           weak_from_this(),                  //
                                           desc,                              //
                                           std::move(pipeline),               //
                                           std::move(pipeline_layout.value),  //
                                           std::move(descs_layout),           //
                                           pipeline_key);
}

// |PipelineLibrary|
PipelineFuture<PipelineDescriptor> PipelineLibrary::GetPipeline(
    PipelineDescriptor descriptor,
    bool async,
    bool threadsafe) {
  Lock lock(pipelines_mutex_);
  if (auto found = pipelines_.find(descriptor); found != pipelines_.end()) {
    return found->second;
  }

  cache_dirty_ = true;
  if (!IsValid()) {
    return {
        descriptor,
        RealizedFuture<std::shared_ptr<Pipeline<PipelineDescriptor>>>(nullptr)};
  }

  auto promise = std::make_shared<
      NoExceptionPromise<std::shared_ptr<Pipeline<PipelineDescriptor>>>>();
  auto pipeline_future =
      PipelineFuture<PipelineDescriptor>{descriptor, promise->get_future()};
  pipelines_[descriptor] = pipeline_future;

  auto weak_this = weak_from_this();

  PipelineKey next_key = pipeline_key_++;
  auto generation_task = [descriptor, weak_this, promise, next_key]() {
    auto thiz = weak_this.lock();
    if (!thiz) {
      promise->set_value(nullptr);
      return;
    }

    promise->set_value(PipelineVK::Create(descriptor,                   //
                                          thiz->device_holder_.lock(),  //
                                          weak_this,                    //
                                          next_key                      //
                                          ));
  };

  if (async) {
    compile_queue_->PostJobForDescriptor(descriptor,
                                         std::move(generation_task));
  } else {
    generation_task();
  }

  return pipeline_future;
}

// |PipelineLibrary|
PipelineFuture<ComputePipelineDescriptor> PipelineLibrary::GetPipeline(
    ComputePipelineDescriptor descriptor,
    bool async) {
  Lock lock(pipelines_mutex_);
  if (auto found = compute_pipelines_.find(descriptor);
      found != compute_pipelines_.end()) {
    return found->second;
  }

  cache_dirty_ = true;
  if (!IsValid()) {
    return {
        descriptor,
        RealizedFuture<std::shared_ptr<Pipeline<ComputePipelineDescriptor>>>(
            nullptr)};
  }

  auto promise = std::make_shared<
      std::promise<std::shared_ptr<Pipeline<ComputePipelineDescriptor>>>>();
  auto pipeline_future = PipelineFuture<ComputePipelineDescriptor>{
      descriptor, promise->get_future()};
  compute_pipelines_[descriptor] = pipeline_future;

  auto weak_this = weak_from_this();

  PipelineKey next_key = pipeline_key_++;
  auto generation_task = [descriptor, weak_this, promise, next_key]() {
    auto self = weak_this.lock();
    if (!self) {
      promise->set_value(nullptr);
      LOG(ERROR) << "Pipeline library was collected before the pipeline "
                    "could be created.";
      return;
    }

    auto pipeline = self->CreateComputePipeline(descriptor, next_key);
    if (!pipeline) {
      promise->set_value(nullptr);
      LOG(ERROR) << "Could not create pipeline: " << descriptor.GetLabel();
      return;
    }

    promise->set_value(std::move(pipeline));
  };

  if (async) {
    worker_task_runner_->PostTask(generation_task);
  } else {
    generation_task();
  }

  return pipeline_future;
}

// |PipelineLibrary|
bool PipelineLibrary::HasPipeline(const PipelineDescriptor& descriptor) {
  Lock lock(pipelines_mutex_);
  return pipelines_.find(descriptor) != pipelines_.end();
}

// |PipelineLibrary|
void PipelineLibrary::RemovePipelinesWithEntryPoint(
    std::shared_ptr<const ShaderFunction> function) {
  Lock lock(pipelines_mutex_);

  fml::erase_if(pipelines_, [&](auto item) {
    return item->first.GetEntrypointForStage(function->GetStage())
        ->IsEqual(*function);
  });
}

void PipelineLibrary::DidAcquireSurfaceFrame() {
  if (++frames_acquired_ == 50u) {
    if (cache_dirty_) {
      cache_dirty_ = false;
      PersistPipelineCacheToDisk();
    }
    frames_acquired_ = 0;
  }
}

void PipelineLibrary::PersistPipelineCacheToDisk() {
  worker_task_runner_->PostTask(
      [weak_cache = decltype(pso_cache_)::weak_type(pso_cache_)]() {
        auto cache = weak_cache.lock();
        if (!cache) {
          return;
        }
        cache->PersistCacheToDisk();
      });
}

const std::shared_ptr<PipelineCache>& PipelineLibrary::GetPSOCache() const {
  return pso_cache_;
}

const std::shared_ptr<fml::ConcurrentTaskRunner>&
PipelineLibrary::GetWorkerTaskRunner() const {
  return worker_task_runner_;
}

PipelineCompileQueue* PipelineLibrary::GetPipelineCompileQueue() const {
  return compile_queue_.get();
}

PipelineFuture<PipelineDescriptor> PipelineLibrary::GetPipeline(
    std::optional<PipelineDescriptor> descriptor,
    bool async) {
  if (descriptor.has_value()) {
    return GetPipeline(descriptor.value(), async);
  }
  auto promise = std::make_shared<
      std::promise<std::shared_ptr<Pipeline<PipelineDescriptor>>>>();
  promise->set_value(nullptr);
  return {descriptor, promise->get_future()};
}

PipelineFuture<ComputePipelineDescriptor> PipelineLibrary::GetPipeline(
    std::optional<ComputePipelineDescriptor> descriptor,
    bool async) {
  if (descriptor.has_value()) {
    return GetPipeline(descriptor.value(), async);
  }
  auto promise = std::make_shared<
      std::promise<std::shared_ptr<Pipeline<ComputePipelineDescriptor>>>>();
  promise->set_value(nullptr);
  return {descriptor, promise->get_future()};
}

void PipelineLibrary::LogPipelineCreation(const PipelineDescriptor& p) {
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG || \
    FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_PROFILE
  WriterLock lock(pipeline_use_counts_mutex_);
  if (!pipeline_use_counts_.contains(p)) {
    pipeline_use_counts_[p] = 0;
  }
#endif
}

void PipelineLibrary::LogPipelineUsage(const PipelineDescriptor& p) {
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG || \
    FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_PROFILE
  WriterLock lock(pipeline_use_counts_mutex_);
  ++pipeline_use_counts_[p];
#endif
}

std::unordered_map<PipelineDescriptor,
                   int,
                   ComparableHash<PipelineDescriptor>,
                   ComparableEqual<PipelineDescriptor>>
PipelineLibrary::GetPipelineUseCounts() const {
  std::unordered_map<PipelineDescriptor, int,
                     ComparableHash<PipelineDescriptor>,
                     ComparableEqual<PipelineDescriptor>>
      counts;
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG || \
    FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_PROFILE
  ReaderLock lock(pipeline_use_counts_mutex_);
  counts = pipeline_use_counts_;
#endif
  return counts;
}

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <unordered_map>

#include "base/thread.h"
#include "base/thread_safety.h"
#include "fml/concurrent_message_loop.h"
#include "fml/unique_fd.h"
#include "renderer/backend/vulkan/capabilities_vk.h"
#include "renderer/backend/vulkan/compute_pipeline_vk.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/pipeline_cache_vk.h"
#include "renderer/backend/vulkan/pipeline_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/compute_pipeline_descriptor.h"
#include "renderer/pipeline.h"
#include "renderer/pipeline_compile_queue.h"
#include "renderer/pipeline_descriptor.h"

namespace ogre {

class Context;

using PipelineMap = std::unordered_map<PipelineDescriptor,
                                       PipelineFuture<PipelineDescriptor>,
                                       ComparableHash<PipelineDescriptor>,
                                       ComparableEqual<PipelineDescriptor>>;

using ComputePipelineMap =
    std::unordered_map<ComputePipelineDescriptor,
                       PipelineFuture<ComputePipelineDescriptor>,
                       ComparableHash<ComputePipelineDescriptor>,
                       ComparableEqual<ComputePipelineDescriptor>>;

class PipelineLibrary final
    : public std::enable_shared_from_this<PipelineLibrary> {
 public:
  ~PipelineLibrary();

  void DidAcquireSurfaceFrame();

  const std::shared_ptr<PipelineCache>& GetPSOCache() const;

  const std::shared_ptr<fml::ConcurrentTaskRunner>& GetWorkerTaskRunner() const;

  bool IsValid() const;

  PipelineFuture<PipelineDescriptor> GetPipeline(
      std::optional<PipelineDescriptor> descriptor,
      bool async = true);

  PipelineFuture<ComputePipelineDescriptor> GetPipeline(
      std::optional<ComputePipelineDescriptor> descriptor,
      bool async = true);

  PipelineFuture<PipelineDescriptor> GetPipeline(PipelineDescriptor descriptor,
                                                 bool async = true,
                                                 bool threadsafe = false);

  PipelineFuture<ComputePipelineDescriptor> GetPipeline(
      ComputePipelineDescriptor descriptor,
      bool async = true);

  bool HasPipeline(const PipelineDescriptor& descriptor);

  void RemovePipelinesWithEntryPoint(
      std::shared_ptr<const ShaderFunction> function);

  PipelineCompileQueue* GetPipelineCompileQueue() const;

  void LogPipelineUsage(const PipelineDescriptor& p);

  void LogPipelineCreation(const PipelineDescriptor& p);

  std::unordered_map<PipelineDescriptor,
                     int,
                     ComparableHash<PipelineDescriptor>,
                     ComparableEqual<PipelineDescriptor>>
  GetPipelineUseCounts() const;

 private:
  friend Context;

  std::weak_ptr<DeviceHolder> device_holder_;
  std::shared_ptr<PipelineCache> pso_cache_;
  std::shared_ptr<fml::ConcurrentTaskRunner> worker_task_runner_;
  Mutex pipelines_mutex_;
  PipelineMap pipelines_ IPLR_GUARDED_BY(pipelines_mutex_);
  ComputePipelineMap compute_pipelines_ IPLR_GUARDED_BY(pipelines_mutex_);
  std::atomic_size_t frames_acquired_ = 0u;
  PipelineKey pipeline_key_ IPLR_GUARDED_BY(pipelines_mutex_) = 1;
  bool is_valid_ = false;
  bool cache_dirty_ = false;
  std::shared_ptr<PipelineCompileQueue> compile_queue_;

#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG || \
    FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_PROFILE
  mutable RWMutex pipeline_use_counts_mutex_;

  std::unordered_map<PipelineDescriptor,
                     int,
                     ComparableHash<PipelineDescriptor>,
                     ComparableEqual<PipelineDescriptor>>
      pipeline_use_counts_ IPLR_GUARDED_BY(pipeline_use_counts_mutex_);
#endif

  PipelineLibrary(
      const std::shared_ptr<DeviceHolder>& device_holder,
      std::shared_ptr<const Capabilities> caps,
      fml::UniqueFD cache_directory,
      std::shared_ptr<fml::ConcurrentTaskRunner> worker_task_runner);

  std::unique_ptr<ComputePipeline> CreateComputePipeline(
      const ComputePipelineDescriptor& desc,
      PipelineKey pipeline_key);

  void PersistPipelineCacheToDisk();

  PipelineLibrary(const PipelineLibrary&) = delete;

  PipelineLibrary& operator=(const PipelineLibrary&) = delete;
};

}  // namespace ogre

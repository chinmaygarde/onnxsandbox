// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/thread.h"
#include "fml/file.h"
#include "renderer/backend/vulkan/capabilities_vk.h"
#include "renderer/backend/vulkan/device_holder_vk.h"

namespace ogre {

class PipelineCache {
 public:
  // The [device] is passed in directly so that it can be used in the
  // constructor directly. The [device_holder] isn't guaranteed to be valid
  // at the time of executing `PipelineCache` because of how `ContextVK` does
  // initialization.
  explicit PipelineCache(std::shared_ptr<const Capabilities> caps,
                         std::shared_ptr<DeviceHolder> device_holder,
                         fml::UniqueFD cache_directory);

  ~PipelineCache();

  bool IsValid() const;

  vk::UniquePipeline CreatePipeline(const vk::GraphicsPipelineCreateInfo& info);

  vk::UniquePipeline CreatePipeline(const vk::ComputePipelineCreateInfo& info);

  const Capabilities* GetCapabilities() const;

  void PersistCacheToDisk();

 private:
  const std::shared_ptr<const Capabilities> caps_;
  std::weak_ptr<DeviceHolder> device_holder_;
  const fml::UniqueFD cache_directory_;
  vk::UniquePipelineCache cache_;
  bool is_valid_ = false;
  Mutex persist_mutex_;

  PipelineCache(const PipelineCache&) = delete;

  PipelineCache& operator=(const PipelineCache&) = delete;
};

}  // namespace ogre

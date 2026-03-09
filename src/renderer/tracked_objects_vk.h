// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "renderer/context_vk.h"
#include "renderer/descriptor_pool_vk.h"
#include "renderer/device_buffer_vk.h"
#include "renderer/gpu_tracer_vk.h"
#include "renderer/texture_source_vk.h"

namespace ogre {

/// @brief A per-frame object used to track resource lifetimes and allocate
///        command buffers and descriptor sets.
class TrackedObjects {
 public:
  explicit TrackedObjects(const std::weak_ptr<const Context>& context,
                          const std::shared_ptr<CommandPool>& pool,
                          std::shared_ptr<DescriptorPool> descriptor_pool,
                          std::unique_ptr<GPUProbe> probe);

  ~TrackedObjects();

  bool IsValid() const;

  void Track(const std::shared_ptr<SharedObject>& object);

  void Track(const std::shared_ptr<const DeviceBufferVK>& buffer);

  void Track(const std::shared_ptr<const TextureSource>& texture);

  vk::CommandBuffer GetCommandBuffer() const;

  DescriptorPool& GetDescriptorPool();

  GPUProbe& GetGPUProbe() const;

 private:
  std::shared_ptr<DescriptorPool> desc_pool_;
  // `shared_ptr` since command buffers have a link to the command pool.
  std::shared_ptr<CommandPool> pool_;
  vk::UniqueCommandBuffer buffer_;
  std::vector<std::shared_ptr<SharedObject>> tracked_objects_;
  std::vector<std::shared_ptr<const DeviceBufferVK>> tracked_buffers_;
  std::vector<std::shared_ptr<const TextureSource>> tracked_textures_;
  std::unique_ptr<GPUProbe> probe_;
  bool is_valid_ = false;

  TrackedObjects(const TrackedObjects&) = delete;

  TrackedObjects& operator=(const TrackedObjects&) = delete;
};

}  // namespace ogre

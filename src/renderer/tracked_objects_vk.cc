// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/tracked_objects_vk.h"

#include "renderer/command_pool_vk.h"
#include "renderer/gpu_tracer_vk.h"

namespace ogre {

TrackedObjects::TrackedObjects(const std::weak_ptr<const Context>& context,
                               const std::shared_ptr<CommandPool>& pool,
                               std::shared_ptr<DescriptorPool> descriptor_pool,
                               std::unique_ptr<GPUProbe> probe)
    : desc_pool_(std::move(descriptor_pool)), probe_(std::move(probe)) {
  if (!pool) {
    return;
  }
  auto buffer = pool->CreateCommandBuffer();
  if (!buffer) {
    return;
  }
  pool_ = pool;
  buffer_ = std::move(buffer);
  is_valid_ = true;
  // Starting values were selected by looking at values from
  // AiksTest.CanRenderMultipleBackdropBlurWithSingleBackdropId.
  tracked_objects_.reserve(5);
  tracked_buffers_.reserve(5);
  tracked_textures_.reserve(5);
}

TrackedObjects::~TrackedObjects() {
  if (!buffer_) {
    return;
  }
  pool_->CollectCommandBuffer(std::move(buffer_));
}

bool TrackedObjects::IsValid() const {
  return is_valid_;
}

void TrackedObjects::Track(const std::shared_ptr<SharedObject>& object) {
  if (!object || (!tracked_objects_.empty() &&
                  object.get() == tracked_objects_.back().get())) {
    return;
  }
  tracked_objects_.emplace_back(object);
}

void TrackedObjects::Track(
    const std::shared_ptr<const DeviceBufferVK>& buffer) {
  if (!buffer || (!tracked_buffers_.empty() &&
                  buffer.get() == tracked_buffers_.back().get())) {
    return;
  }
  tracked_buffers_.emplace_back(buffer);
}

void TrackedObjects::Track(
    const std::shared_ptr<const TextureSource>& texture) {
  if (!texture || (!tracked_textures_.empty() &&
                   texture.get() == tracked_textures_.back().get())) {
    return;
  }
  tracked_textures_.emplace_back(texture);
}

vk::CommandBuffer TrackedObjects::GetCommandBuffer() const {
  return *buffer_;
}

DescriptorPool& TrackedObjects::GetDescriptorPool() {
  return *desc_pool_;
}

GPUProbe& TrackedObjects::GetGPUProbe() const {
  return *probe_.get();
}

}  // namespace ogre

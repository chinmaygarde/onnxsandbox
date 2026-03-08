// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/backend/vulkan/command_buffer_vk.h"

#include <memory>
#include <utility>

#include "fml/logging.h"
#include "renderer/backend/vulkan/blit_pass_vk.h"
#include "renderer/backend/vulkan/compute_pass_vk.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/descriptor_pool_vk.h"
#include "renderer/backend/vulkan/gpu_tracer_vk.h"
#include "renderer/backend/vulkan/render_pass_vk.h"
#include "renderer/backend/vulkan/texture_source_vk.h"
#include "renderer/backend/vulkan/texture_vk.h"
#include "renderer/backend/vulkan/tracked_objects_vk.h"
#include "renderer/render_target.h"

namespace ogre {

CommandBuffer::CommandBuffer(std::weak_ptr<const Context> context,
                             std::weak_ptr<const DeviceHolder> device_holder,
                             std::shared_ptr<TrackedObjects> tracked_objects)
    : context_(std::move(context)),
      device_holder_(std::move(device_holder)),
      tracked_objects_(std::move(tracked_objects)) {}

CommandBuffer::~CommandBuffer() = default;

void CommandBuffer::SetLabel(std::string_view label) const {
#ifdef OGRE_DEBUG
  auto context = context_.lock();
  if (!context) {
    return;
  }
  context->SetDebugName(GetCommandBuffer(), label);
#endif  // OGRE_DEBUG
}

bool CommandBuffer::IsValid() const {
  return true;
}

void CommandBuffer::WaitUntilCompleted() {}

void CommandBuffer::WaitUntilScheduled() {}

std::shared_ptr<RenderPass> CommandBuffer::CreateRenderPass(
    const RenderTarget& render_target) {
  auto context = context_.lock();
  if (!context) {
    return nullptr;
  }
  auto pass = std::shared_ptr<RenderPass>(new RenderPass(context,            //
                                                         render_target,      //
                                                         shared_from_this()  //
                                                         ));
  if (!pass->IsValid()) {
    return nullptr;
  }
  pass->SetLabel("RenderPass");
  return pass;
}

std::shared_ptr<BlitPass> CommandBuffer::CreateBlitPass() {
  if (!IsValid()) {
    return nullptr;
  }
  auto context = context_.lock();
  if (!context) {
    return nullptr;
  }
  auto pass = std::shared_ptr<BlitPass>(
      new BlitPass(shared_from_this(), context->GetWorkarounds()));
  if (!pass->IsValid()) {
    return nullptr;
  }
  pass->SetLabel("BlitPass");
  return pass;
}

std::shared_ptr<ComputePass> CommandBuffer::CreateComputePass() {
  if (!IsValid()) {
    return nullptr;
  }
  auto context = context_.lock();
  if (!context) {
    return nullptr;
  }
  auto pass =
      std::shared_ptr<ComputePass>(new ComputePass(context,            //
                                                   shared_from_this()  //
                                                   ));
  if (!pass->IsValid()) {
    return nullptr;
  }
  pass->SetLabel("ComputePass");
  return pass;
}

bool CommandBuffer::EndCommandBuffer() const {
  InsertDebugMarker("QueueSubmit");

  auto command_buffer = GetCommandBuffer();
  tracked_objects_->GetGPUProbe().RecordCmdBufferEnd(command_buffer);

  auto status = command_buffer.end();
  if (status != vk::Result::eSuccess) {
    VALIDATION_LOG << "Failed to end command buffer: " << vk::to_string(status);
    return false;
  }
  return true;
}

vk::CommandBuffer CommandBuffer::GetCommandBuffer() const {
  if (tracked_objects_) {
    return tracked_objects_->GetCommandBuffer();
  }
  return {};
}

bool CommandBuffer::Track(const std::shared_ptr<SharedObject>& object) {
  if (!IsValid()) {
    return false;
  }
  tracked_objects_->Track(object);
  return true;
}

bool CommandBuffer::Track(const std::shared_ptr<const DeviceBuffer>& buffer) {
  if (!IsValid()) {
    return false;
  }
  tracked_objects_->Track(buffer);
  return true;
}

bool CommandBuffer::Track(const std::shared_ptr<const TextureSource>& texture) {
  if (!IsValid()) {
    return false;
  }
  tracked_objects_->Track(texture);
  return true;
}

bool CommandBuffer::Track(const std::shared_ptr<const Texture>& texture) {
  if (!IsValid()) {
    return false;
  }
  if (!texture) {
    return true;
  }
  return Track(TextureVK::Cast(*texture).GetTextureSource());
}

fml::StatusOr<vk::DescriptorSet> CommandBuffer::AllocateDescriptorSets(
    const vk::DescriptorSetLayout& layout,
    PipelineKey pipeline_key,
    const Context& context) {
  if (!IsValid()) {
    return fml::Status(fml::StatusCode::kUnknown, "command encoder invalid");
  }

  return tracked_objects_->GetDescriptorPool().AllocateDescriptorSets(
      layout, pipeline_key, context);
}

void CommandBuffer::PushDebugGroup(std::string_view label) const {
  if (!HasValidationLayers()) {
    return;
  }
  vk::DebugUtilsLabelEXT label_info;
  label_info.pLabelName = label.data();
  if (auto command_buffer = GetCommandBuffer()) {
    command_buffer.beginDebugUtilsLabelEXT(label_info);
  }
}

void CommandBuffer::PopDebugGroup() const {
  if (!HasValidationLayers()) {
    return;
  }
  if (auto command_buffer = GetCommandBuffer()) {
    command_buffer.endDebugUtilsLabelEXT();
  }
}

void CommandBuffer::InsertDebugMarker(std::string_view label) const {
  if (!HasValidationLayers()) {
    return;
  }
  vk::DebugUtilsLabelEXT label_info;
  label_info.pLabelName = label.data();
  if (auto command_buffer = GetCommandBuffer()) {
    command_buffer.insertDebugUtilsLabelEXT(label_info);
  }
}

DescriptorPool& CommandBuffer::GetDescriptorPool() const {
  return tracked_objects_->GetDescriptorPool();
}

}  // namespace ogre

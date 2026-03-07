// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_

#include <functional>
#include <memory>
#include <string>

#include "fml/status_or.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/pipeline.h"

namespace ogre {

class BlitPass;
class ComputePassVK;
class ContextVK;
class DescriptorPool;
class DeviceBuffer;
class RenderPassVK;
class RenderTarget;
class SharedObjectVK;
class Texture;
class TextureSource;
class TrackedObjectsVK;

class CommandBuffer final : public std::enable_shared_from_this<CommandBuffer> {
 public:
  enum class Status {
    kPending,
    kError,
    kCompleted,
  };

  using CompletionCallback = std::function<void(Status)>;

  ~CommandBuffer();

  bool IsValid() const;

  void SetLabel(std::string_view label) const;

  void WaitUntilCompleted();

  void WaitUntilScheduled();

  std::shared_ptr<RenderPassVK> CreateRenderPass(
      const RenderTarget& render_target);

  std::shared_ptr<BlitPass> CreateBlitPass();

  std::shared_ptr<ComputePassVK> CreateComputePass();

  // Encoder Functionality

  /// @brief Ensure that [object] is kept alive until this command buffer
  ///        completes execution.
  bool Track(const std::shared_ptr<SharedObjectVK>& object);

  /// @brief Ensure that [buffer] is kept alive until this command buffer
  ///        completes execution.
  bool Track(const std::shared_ptr<const DeviceBuffer>& buffer);

  /// @brief Ensure that [texture] is kept alive until this command buffer
  ///       completes execution.
  bool Track(const std::shared_ptr<const Texture>& texture);

  /// @brief Ensure that [texture] is kept alive until this command buffer
  ///        completes execution.
  bool Track(const std::shared_ptr<const TextureSource>& texture);

  /// @brief Retrieve the native command buffer from this object.
  vk::CommandBuffer GetCommandBuffer() const;

  /// @brief Push a debug group.
  ///
  /// This label is only visible in debuggers like RenderDoc. This function is
  /// ignored in release builds.
  void PushDebugGroup(std::string_view label) const;

  /// @brief Pop the previous debug group.
  ///
  /// This label is only visible in debuggers like RenderDoc. This function is
  /// ignored in release builds.
  void PopDebugGroup() const;

  /// @brief Insert a new debug marker.
  ///
  /// This label is only visible in debuggers like RenderDoc. This function is
  /// ignored in release builds.
  void InsertDebugMarker(std::string_view label) const;

  /// @brief End recording of the current command buffer.
  bool EndCommandBuffer() const;

  /// @brief Allocate a new descriptor set for the given [layout].
  fml::StatusOr<vk::DescriptorSet> AllocateDescriptorSets(
      const vk::DescriptorSetLayout& layout,
      PipelineKey pipeline_key,
      const ContextVK& context);

  // Visible for testing.
  DescriptorPool& GetDescriptorPool() const;

 private:
  friend class ContextVK;
  friend class CommandQueue;

  std::weak_ptr<const ContextVK> context_;
  std::weak_ptr<const DeviceHolderVK> device_holder_;
  std::shared_ptr<TrackedObjectsVK> tracked_objects_;

  CommandBuffer(std::weak_ptr<const ContextVK> context,
                std::weak_ptr<const DeviceHolderVK> device_holder,
                std::shared_ptr<TrackedObjectsVK> tracked_objects);

  CommandBuffer(const CommandBuffer&) = delete;

  CommandBuffer& operator=(const CommandBuffer&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_

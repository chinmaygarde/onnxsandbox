// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "fml/status_or.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/pipeline.h"

namespace ogre {

class BlitPass;
class ComputePass;
class Context;
class DescriptorPool;
class DeviceBufferVK;
class RenderPass;
class RenderTarget;
class SharedObject;
class Texture;
class TextureSource;
class TrackedObjects;

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

  std::shared_ptr<RenderPass> CreateRenderPass(
      const RenderTarget& render_target);

  std::shared_ptr<BlitPass> CreateBlitPass();

  std::shared_ptr<ComputePass> CreateComputePass();

  // Encoder Functionality

  /// @brief Ensure that [object] is kept alive until this command buffer
  ///        completes execution.
  bool Track(const std::shared_ptr<SharedObject>& object);

  /// @brief Ensure that [buffer] is kept alive until this command buffer
  ///        completes execution.
  bool Track(const std::shared_ptr<const DeviceBufferVK>& buffer);

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
      const Context& context);

  // Visible for testing.
  DescriptorPool& GetDescriptorPool() const;

 private:
  friend class Context;
  friend class CommandQueue;

  std::weak_ptr<const Context> context_;
  std::weak_ptr<const DeviceHolder> device_holder_;
  std::shared_ptr<TrackedObjects> tracked_objects_;

  CommandBuffer(std::weak_ptr<const Context> context,
                std::weak_ptr<const DeviceHolder> device_holder,
                std::shared_ptr<TrackedObjects> tracked_objects);

  CommandBuffer(const CommandBuffer&) = delete;

  CommandBuffer& operator=(const CommandBuffer&) = delete;
};

}  // namespace ogre

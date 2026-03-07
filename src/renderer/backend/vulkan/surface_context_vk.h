// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_

#include <memory>

#include "base/backend_cast.h"
#include "core/runtime_types.h"
#include "renderer/backend/vulkan/command_queue_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/context.h"

namespace ogre {

class ContextVK;
class SurfaceVK;
class Swapchain;

/// For Vulkan, there is both a ContextVK that implements Context and a
/// SurfaceContext that also implements Context and takes a ContextVK as its
/// parent. There is a one to many relationship between ContextVK and
/// SurfaceContext.
///
/// Most operations in this class are delegated to the parent ContextVK.
/// This class specifically manages swapchains and creation of VkSurfaces on
/// Android. By maintaining the swapchain this way, it is possible to have
/// multiple surfaces sharing the same ContextVK without stepping on each
/// other's swapchains.
class SurfaceContext : public Context,
                       public BackendCast<SurfaceContext, Context> {
 public:
  explicit SurfaceContext(const std::shared_ptr<ContextVK>& parent);

  // |Context|
  ~SurfaceContext() override;

  // |Context|
  BackendType GetBackendType() const override;

  // |Context|
  std::string DescribeGpuModel() const override;

  // |Context|
  bool IsValid() const override;

  // |Context|
  std::shared_ptr<Allocator> GetResourceAllocator() const override;

  // |Context|
  std::shared_ptr<ShaderLibraryVK> GetShaderLibrary() const override;

  // |Context|
  std::shared_ptr<SamplerLibraryVK> GetSamplerLibrary() const override;

  // |Context|
  std::shared_ptr<PipelineLibraryVK> GetPipelineLibrary() const override;

  // |Context|
  std::shared_ptr<CommandBuffer> CreateCommandBuffer() const override;

  // |Context|
  const std::shared_ptr<const Capabilities>& GetCapabilities() const override;

  // |Context|
  std::shared_ptr<CommandQueue> GetCommandQueue() const override;

  // |Context|
  std::shared_ptr<const IdleWaiterVK> GetIdleWaiter() const override;

  // |Context|
  RuntimeStageBackend GetRuntimeStageBackend() const override;

  // |Context|
  bool SubmitOnscreen(std::shared_ptr<CommandBuffer> cmd_buffer) override;

  // |Context|
  void Shutdown() override;

  [[nodiscard]] bool SetWindowSurface(vk::UniqueSurfaceKHR surface,
                                      const ISize& size);

  [[nodiscard]] bool SetSwapchain(std::shared_ptr<Swapchain> swapchain);

  std::unique_ptr<SurfaceVK> AcquireNextSurface();

  /// @brief Performs frame incrementing processes like AcquireNextSurface but
  ///        without the surface.
  ///
  /// Used by the embedder.h implementations.
  void MarkFrameEnd();

  /// @brief Mark the current swapchain configuration as dirty, forcing it to be
  ///        recreated on the next frame.
  void UpdateSurfaceSize(const ISize& size) const;

  /// @brief Can be called when the surface is destroyed to reduce memory usage.
  void TeardownSwapchain();

  // |Context|
  void InitializeCommonlyUsedShadersIfNeeded() const override;

  // |Context|
  void DisposeThreadLocalCachedResources() override;

  const vk::Device& GetDevice() const;

  const std::shared_ptr<ContextVK>& GetParent() const;

  bool EnqueueCommandBuffer(
      std::shared_ptr<CommandBuffer> command_buffer) override;

  bool FlushCommandBuffers() override;

 private:
  std::shared_ptr<ContextVK> parent_;
  std::shared_ptr<Swapchain> swapchain_;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_

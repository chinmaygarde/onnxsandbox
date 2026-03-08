// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_

#include <memory>

#include "geometry/size.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/swapchain/swapchain_vk.h"
#include "renderer/backend/vulkan/vk.h"

namespace ogre {

class KHRSwapchainImpl;

//------------------------------------------------------------------------------
/// @brief      A swapchain implemented backed by VK_KHR_swapchain and
///             VK_KHR_surface.
///
class KHRSwapchain final : public Swapchain {
 public:
  ~KHRSwapchain();

  // |Swapchain|
  bool IsValid() const override;

  // |Swapchain|
  std::unique_ptr<Surface> AcquireNextDrawable() override;

  // |Swapchain|
  vk::Format GetSurfaceFormat() const override;

  // |Swapchain|
  void UpdateSurfaceSize(const ISize& size) override;

  // |Swapchain|
  void AddFinalCommandBuffer(
      std::shared_ptr<CommandBuffer> cmd_buffer) const override;

 private:
  friend class Swapchain;

  std::shared_ptr<KHRSwapchainImpl> impl_;
  ISize size_;
  const bool enable_msaa_;

  KHRSwapchain(const std::shared_ptr<Context>& context,
               vk::UniqueSurfaceKHR surface,
               const ISize& size,
               bool enable_msaa);

  KHRSwapchain(const KHRSwapchain&) = delete;

  KHRSwapchain& operator=(const KHRSwapchain&) = delete;

  std::unique_ptr<Surface> AcquireNextDrawable(size_t resize_retry_count);
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_

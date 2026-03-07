// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_

#include <memory>

#include "geometry/size.h"
#include "renderer/backend/vulkan/swapchain/swapchain_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/context.h"
#include "renderer/surface.h"

namespace ogre {

class KHRSwapchainImplVK;

//------------------------------------------------------------------------------
/// @brief      A swapchain implemented backed by VK_KHR_swapchain and
///             VK_KHR_surface.
///
class KHRSwapchainVK final : public SwapchainVK {
 public:
  ~KHRSwapchainVK();

  // |SwapchainVK|
  bool IsValid() const override;

  // |SwapchainVK|
  std::unique_ptr<Surface> AcquireNextDrawable() override;

  // |SwapchainVK|
  vk::Format GetSurfaceFormat() const override;

  // |SwapchainVK|
  void UpdateSurfaceSize(const ISize& size) override;

  // |SwapchainVK|
  void AddFinalCommandBuffer(
      std::shared_ptr<CommandBuffer> cmd_buffer) const override;

 private:
  friend class SwapchainVK;

  std::shared_ptr<KHRSwapchainImplVK> impl_;
  ISize size_;
  const bool enable_msaa_;

  KHRSwapchainVK(const std::shared_ptr<Context>& context,
                 vk::UniqueSurfaceKHR surface,
                 const ISize& size,
                 bool enable_msaa);

  KHRSwapchainVK(const KHRSwapchainVK&) = delete;

  KHRSwapchainVK& operator=(const KHRSwapchainVK&) = delete;

  std::unique_ptr<Surface> AcquireNextDrawable(size_t resize_retry_count);
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_VK_H_

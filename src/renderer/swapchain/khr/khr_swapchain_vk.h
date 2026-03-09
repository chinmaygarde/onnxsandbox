// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "geometry/size.h"
#include "renderer/context_vk.h"
#include "renderer/swapchain/swapchain_vk.h"
#include "renderer/vk.h"

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

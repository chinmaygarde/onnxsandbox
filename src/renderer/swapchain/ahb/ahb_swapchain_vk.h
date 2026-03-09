// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "renderer/swapchain/ahb/ahb_swapchain_impl_vk.h"
#include "renderer/swapchain/swapchain_vk.h"
#include "toolkit/android/native_window.h"
#include "toolkit/android/surface_control.h"
#include "toolkit/android/surface_transaction.h"

namespace ogre {

using CreateTransactionCB = std::function<android::SurfaceTransaction()>;

//------------------------------------------------------------------------------
/// @brief      The implementation of a swapchain that uses hardware buffers
///             presented to a given surface control on Android.
///
/// @warning    This swapchain implementation is not available on all Android
///             versions supported by Flutter. Perform the
///             `IsAvailableOnPlatform` check and fallback to KHR swapchains if
///             this type of swapchain cannot be created. The available building
///             blocks for these kinds of swapchains are only available on
///             Android API levels >= 29.
///
class AHBSwapchain final : public Swapchain {
 public:
  static bool IsAvailableOnPlatform();

  // |Swapchain|
  ~AHBSwapchain() override;

  AHBSwapchain(const AHBSwapchain&) = delete;

  AHBSwapchain& operator=(const AHBSwapchain&) = delete;

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

  std::weak_ptr<Context> context_;
  std::shared_ptr<android::SurfaceControl> surface_control_;
  const bool enable_msaa_;
  CreateTransactionCB cb_;
  std::shared_ptr<AHBSwapchainImpl> impl_;

  explicit AHBSwapchain(const std::shared_ptr<Context>& context,
                        ANativeWindow* window,
                        const CreateTransactionCB& cb,
                        const ISize& size,
                        bool enable_msaa);
};

}  // namespace ogre

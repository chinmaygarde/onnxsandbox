// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_VK_H_

#include <memory>

#include "fml/build_config.h"
#include "geometry/size.h"
#include "renderer/backend/vulkan/command_buffer_vk.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/swapchain/surface_vk.h"
#include "renderer/backend/vulkan/vk.h"

#if FML_OS_ANDROID
#include "toolkit/android/native_window.h"
#include "toolkit/android/surface_transaction.h"
#endif  // FML_OS_ANDROID

namespace ogre {

#if FML_OS_ANDROID
using CreateTransactionCB = std::function<android::SurfaceTransaction()>;
#endif  // FML_OS_ANDROID

//------------------------------------------------------------------------------
/// @brief      A swapchain that adapts to the underlying surface going out of
///             date. If the caller cannot acquire the next drawable, it is due
///             to an unrecoverable error and the swapchain must be recreated
///             with a new surface.
///
class Swapchain {
 public:
  static std::shared_ptr<Swapchain> Create(
      const std::shared_ptr<Context>& context,
      vk::UniqueSurfaceKHR surface,
      const ISize& size,
      bool enable_msaa = true);

#if FML_OS_ANDROID
  static std::shared_ptr<Swapchain> Create(
      const std::shared_ptr<Context>& context,
      ANativeWindow* window,
      const CreateTransactionCB& cb,
      bool enable_msaa = true);
#endif  // FML_OS_ANDROID

  virtual ~Swapchain();

  Swapchain(const Swapchain&) = delete;

  Swapchain& operator=(const Swapchain&) = delete;

  virtual bool IsValid() const = 0;

  virtual std::unique_ptr<Surface> AcquireNextDrawable() = 0;

  virtual vk::Format GetSurfaceFormat() const = 0;

  virtual void AddFinalCommandBuffer(
      std::shared_ptr<CommandBuffer> cmd_buffer) const = 0;

  /// @brief Mark the current swapchain configuration as dirty, forcing it to be
  ///        recreated on the next frame.
  virtual void UpdateSurfaceSize(const ISize& size) = 0;

 protected:
  Swapchain();
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_VK_H_

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <memory>

#include "geometry/size.h"
#include "renderer/swapchain/swapchain_transients_vk.h"
#include "renderer/vk.h"

namespace ogre {

class CommandBuffer;
class Context;
class KHRSwapchainImage;
class Surface;
struct KHRFrameSynchronizer;

//------------------------------------------------------------------------------
/// @brief      An instance of a swapchain that does NOT adapt to going out of
///             date with the underlying surface. Errors will be indicated when
///             the next drawable is acquired from this implementation of the
///             swapchain. If the error is due the swapchain going out of date,
///             the caller must recreate another instance by optionally
///             stealing this implementations guts.
///
class KHRSwapchainImpl final
    : public std::enable_shared_from_this<KHRSwapchainImpl> {
 public:
  static std::shared_ptr<KHRSwapchainImpl> Create(
      const std::shared_ptr<Context>& context,
      vk::UniqueSurfaceKHR surface,
      const ISize& size,
      bool enable_msaa = true,
      vk::SwapchainKHR old_swapchain = VK_NULL_HANDLE);

  ~KHRSwapchainImpl();

  bool IsValid() const;

  struct AcquireResult {
    std::unique_ptr<Surface> surface;
    bool out_of_date = false;

    explicit AcquireResult(bool p_out_of_date = false)
        : out_of_date(p_out_of_date) {}

    explicit AcquireResult(std::unique_ptr<Surface> p_surface)
        : surface(std::move(p_surface)) {}
  };

  AcquireResult AcquireNextDrawable();

  vk::Format GetSurfaceFormat() const;

  std::shared_ptr<Context> GetContext() const;

  std::pair<vk::UniqueSurfaceKHR, vk::UniqueSwapchainKHR> DestroySwapchain();

  const ISize& GetSize() const;

  void AddFinalCommandBuffer(std::shared_ptr<CommandBuffer> cmd_buffer);

  std::optional<ISize> GetCurrentUnderlyingSurfaceSize() const;

 private:
  std::weak_ptr<Context> context_;
  vk::UniqueSurfaceKHR surface_;
  vk::Format surface_format_ = vk::Format::eUndefined;
  vk::UniqueSwapchainKHR swapchain_;
  std::shared_ptr<SwapchainTransients> transients_;
  std::vector<std::shared_ptr<KHRSwapchainImage>> images_;
  std::vector<std::unique_ptr<KHRFrameSynchronizer>> synchronizers_;
  std::vector<vk::UniqueSemaphore> present_semaphores_;
  size_t current_frame_ = 0u;
  ISize size_;
  bool enable_msaa_ = true;
  bool is_valid_ = false;

  KHRSwapchainImpl(const std::shared_ptr<Context>& context,
                   vk::UniqueSurfaceKHR surface,
                   const ISize& size,
                   bool enable_msaa,
                   vk::SwapchainKHR old_swapchain);

  bool Present(const std::shared_ptr<KHRSwapchainImage>& image, uint32_t index);

  void WaitIdle() const;

  KHRSwapchainImpl(const KHRSwapchainImpl&) = delete;

  KHRSwapchainImpl& operator=(const KHRSwapchainImpl&) = delete;
};

}  // namespace ogre

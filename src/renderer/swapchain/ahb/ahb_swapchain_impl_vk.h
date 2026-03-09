// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/thread.h"
#include "fml/closure.h"
#include "renderer/android/ahb_texture_source_vk.h"
#include "renderer/swapchain/ahb/ahb_texture_pool_vk.h"
#include "renderer/swapchain/ahb/external_semaphore_vk.h"
#include "renderer/swapchain/surface_vk.h"
#include "renderer/swapchain/swapchain_transients_vk.h"
#include "toolkit/android/hardware_buffer.h"
#include "toolkit/android/surface_control.h"
#include "toolkit/android/surface_transaction.h"
#include "vulkan/vulkan_handles.hpp"

namespace ogre {

class CommandBuffer;

using CreateTransactionCB = std::function<android::SurfaceTransaction()>;

static constexpr const size_t kMaxPendingPresents = 2u;

struct AHBFrameSynchronizer {
  vk::UniqueFence acquire;
  vk::UniqueSemaphore render_ready = {};
  std::shared_ptr<ExternalSemaphore> present_ready;
  std::shared_ptr<CommandBuffer> final_cmd_buffer;
  bool is_valid = false;

  explicit AHBFrameSynchronizer(const vk::Device& device);

  ~AHBFrameSynchronizer();

  bool IsValid() const;

  bool WaitForFence(const vk::Device& device);
};

//------------------------------------------------------------------------------
/// @brief      The implementation of a swapchain at a specific size. Resizes to
///             the surface will cause the instance of the swapchain impl at
///             that size to be discarded along with all its caches and
///             transients.
///
class AHBSwapchainImpl final
    : public std::enable_shared_from_this<AHBSwapchainImpl> {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Create a swapchain of a specific size whose images will be
  ///             presented to the provided surface control.
  ///
  /// @param[in]  context          The context whose allocators will be used to
  ///                              create swapchain image resources.
  /// @param[in]  surface_control  The surface control to which the swapchain
  ///                              images will be presented.
  /// @param[in]  size             The size of the swapchain images. This is
  ///                              constant for the lifecycle of the swapchain
  ///                              impl.
  /// @param[in]  enable_msaa      If the swapchain images will be presented
  ///                              using a render target that enables MSAA. This
  ///                              allows for additional caching of transients.
  ///
  /// @return     A valid swapchain impl if one can be created. `nullptr`
  ///             otherwise.
  ///
  static std::shared_ptr<AHBSwapchainImpl> Create(
      const std::weak_ptr<Context>& context,
      std::weak_ptr<android::SurfaceControl> surface_control,
      const CreateTransactionCB& cb,
      const ISize& size,
      bool enable_msaa);

  ~AHBSwapchainImpl();

  AHBSwapchainImpl(const AHBSwapchainImpl&) = delete;

  AHBSwapchainImpl& operator=(const AHBSwapchainImpl&) = delete;

  //----------------------------------------------------------------------------
  /// @return     The size of the swapchain images that will be displayed on the
  ///             surface control.
  ///
  const ISize& GetSize() const;

  //----------------------------------------------------------------------------
  /// @return     If the swapchain impl is valid. If it is not, the instance
  ///             must be discarded. There is no error recovery.
  ///
  bool IsValid() const;

  //----------------------------------------------------------------------------
  /// @brief      Get the descriptor used to create the hardware buffers that
  ///             will be displayed on the surface control.
  ///
  /// @return     The descriptor.
  ///
  const android::HardwareBufferDescriptor& GetDescriptor() const;

  //----------------------------------------------------------------------------
  /// @brief      Acquire the next surface that can be used to present to the
  ///             swapchain.
  ///
  /// @return     A surface if one can be created. If one cannot be created, it
  ///             is likely due to resource exhaustion.
  ///
  std::unique_ptr<Surface> AcquireNextDrawable();

  void AddFinalCommandBuffer(std::shared_ptr<CommandBuffer> cmd_buffer);

 private:
  using AutoSemaSignaler = std::shared_ptr<fml::ScopedCleanupClosure>;

  std::weak_ptr<android::SurfaceControl> surface_control_;
  android::HardwareBufferDescriptor desc_;
  std::shared_ptr<AHBTexturePool> pool_;
  std::shared_ptr<SwapchainTransients> transients_;

  // In C++20, this mutex can be replaced by the shared pointer specialization
  // of std::atomic.
  Mutex currently_displayed_texture_mutex_;
  std::shared_ptr<AHBTextureSource> currently_displayed_texture_
      IPLR_GUARDED_BY(currently_displayed_texture_mutex_);

  std::vector<std::unique_ptr<AHBFrameSynchronizer>> frame_data_;
  size_t frame_index_ = 0;
  CreateTransactionCB cb_;
  bool is_valid_ = false;

  explicit AHBSwapchainImpl(
      const std::weak_ptr<Context>& context,
      std::weak_ptr<android::SurfaceControl> surface_control,
      const CreateTransactionCB& cb,
      const ISize& size,
      bool enable_msaa);

  bool Present(const std::shared_ptr<AHBTextureSource>& texture);

  vk::UniqueSemaphore CreateRenderReadySemaphore(
      const std::shared_ptr<fml::UniqueFD>& fd) const;

  bool ImportRenderReady(
      const std::shared_ptr<fml::UniqueFD>& render_ready_fence,
      const std::shared_ptr<AHBTextureSource>& texture);

  std::shared_ptr<ExternalSemaphore> SubmitSignalForPresentReady(
      const std::shared_ptr<AHBTextureSource>& texture) const;

  void OnTextureUpdatedOnSurfaceControl(
      std::shared_ptr<AHBTextureSource> texture,
      ASurfaceTransactionStats* stats);
};

}  // namespace ogre

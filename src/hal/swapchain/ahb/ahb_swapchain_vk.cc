// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/swapchain/ahb/ahb_swapchain_vk.h"

#include "fml/trace_event.h"
#include "hal/context_vk.h"
#include "hal/formats_vk.h"
#include "hal/swapchain/ahb/ahb_formats.h"
#include "third_party/vulkan-deps/vulkan-headers/src/include/vulkan/vulkan_enums.hpp"

namespace ogre {

bool AHBSwapchain::IsAvailableOnPlatform() {
  return android::SurfaceControl::IsAvailableOnPlatform() &&
         android::HardwareBuffer::IsAvailableOnPlatform();
}

AHBSwapchain::AHBSwapchain(const std::shared_ptr<Context>& context,
                           ANativeWindow* window,
                           const CreateTransactionCB& cb,
                           const ISize& size,
                           bool enable_msaa)
    : context_(context),
      surface_control_(
          std::make_shared<android::SurfaceControl>(window, "ImpellerSurface")),
      enable_msaa_(enable_msaa),
      cb_(cb) {
  UpdateSurfaceSize(size);
}

AHBSwapchain::~AHBSwapchain() = default;

// |Swapchain|
bool AHBSwapchain::IsValid() const {
  return impl_ ? impl_->IsValid() : false;
}

// |Swapchain|
std::unique_ptr<Surface> AHBSwapchain::AcquireNextDrawable() {
  if (!IsValid()) {
    return nullptr;
  }

  TRACE_EVENT0("ogre", __FUNCTION__);
  return impl_->AcquireNextDrawable();
}

// |Swapchain|
vk::Format AHBSwapchain::GetSurfaceFormat() const {
  return IsValid()
             ? ToVKImageFormat(ToPixelFormat(impl_->GetDescriptor().format))
             : vk::Format::eUndefined;
}

// |Swapchain|
void AHBSwapchain::AddFinalCommandBuffer(
    std::shared_ptr<CommandBuffer> cmd_buffer) const {
  return impl_->AddFinalCommandBuffer(cmd_buffer);
}

// |Swapchain|
void AHBSwapchain::UpdateSurfaceSize(const ISize& size) {
  if (impl_ && impl_->GetSize() == size) {
    return;
  }
  TRACE_EVENT0("ogre", __FUNCTION__);
  auto impl = AHBSwapchainImpl::Create(context_,          //
                                       surface_control_,  //
                                       cb_,               //
                                       size,              //
                                       enable_msaa_       //
  );
  if (!impl || !impl->IsValid()) {
    LOG(ERROR) << "Could not resize swapchain to size: " << size;
    return;
  }
  impl_ = std::move(impl);
}

}  // namespace ogre

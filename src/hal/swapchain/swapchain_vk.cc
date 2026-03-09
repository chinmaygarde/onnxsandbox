// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/swapchain/swapchain_vk.h"

#include "fml/trace_event.h"
#include "hal/context_vk.h"
#include "hal/swapchain/khr/khr_swapchain_vk.h"
#include "hal/vk.h"

#if FML_OS_ANDROID
#include "hal/swapchain/ahb/ahb_swapchain_vk.h"
#endif  // FML_OS_ANDROID

namespace ogre {

std::shared_ptr<Swapchain> Swapchain::Create(
    const std::shared_ptr<Context>& context,
    vk::UniqueSurfaceKHR surface,
    const ISize& size,
    bool enable_msaa) {
  auto swapchain = std::shared_ptr<KHRSwapchain>(
      new KHRSwapchain(context, std::move(surface), size, enable_msaa));
  if (!swapchain->IsValid()) {
    LOG(ERROR) << "Could not create valid swapchain.";
    return nullptr;
  }
  return swapchain;
}

#if FML_OS_ANDROID
std::shared_ptr<Swapchain> Swapchain::Create(
    const std::shared_ptr<Context>& context,
    ANativeWindow* p_window,
    const CreateTransactionCB& cb,
    bool enable_msaa) {
  TRACE_EVENT0("ogre", "CreateAndroidSwapchain");
  if (!context) {
    return nullptr;
  }

  android::NativeWindow window(p_window);
  if (!window.IsValid()) {
    return nullptr;
  }

  // Use AHB Swapchains if they are opted in.
  if ((*context).GetShouldEnableSurfaceControlSwapchain() &&
      AHBSwapchain::IsAvailableOnPlatform()) {
    CreateTransactionCB callback =
        android_get_device_api_level() >= 34 ? cb : CreateTransactionCB({});
    auto ahb_swapchain =
        std::shared_ptr<AHBSwapchain>(new AHBSwapchain(context,             //
                                                       window.GetHandle(),  //
                                                       callback,            //
                                                       window.GetSize(),    //
                                                       enable_msaa          //
                                                       ));

    if (ahb_swapchain->IsValid()) {
      return ahb_swapchain;
    } else {
      LOG(ERROR)
          << "Could not create AHB swapchain. Falling back to KHR variant.";
    }
  }

  vk::AndroidSurfaceCreateInfoKHR surface_info;
  surface_info.setWindow(window.GetHandle());
  auto [result, surface] =
      (*context).GetInstance().createAndroidSurfaceKHRUnique(surface_info);
  if (result != vk::Result::eSuccess) {
    LOG(ERROR) << "Could not create KHR Android Surface: "
               << vk::to_string(result);
    return nullptr;
  }

  // Fallback to KHR swapchains if AHB swapchains aren't available.
  return Create(context, std::move(surface), window.GetSize(), enable_msaa);
}
#endif  // FML_OS_ANDROID

Swapchain::Swapchain() = default;

Swapchain::~Swapchain() = default;

}  // namespace ogre

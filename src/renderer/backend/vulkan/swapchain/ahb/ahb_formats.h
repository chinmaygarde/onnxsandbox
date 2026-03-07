// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_AHB_FORMATS_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_AHB_FORMATS_H_

#include "core/formats.h"
#include "toolkit/android/hardware_buffer.h"

namespace ogre {

constexpr PixelFormat ToPixelFormat(android::HardwareBufferFormat format) {
  switch (format) {
    case android::HardwareBufferFormat::kR8G8B8A8UNormInt:
      return PixelFormat::kR8G8B8A8UNormInt;
  }
  FML_UNREACHABLE();
}

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_AHB_FORMATS_H_

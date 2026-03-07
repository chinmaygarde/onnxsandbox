// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/backend/vulkan/yuv_conversion_library_vk.h"

#include "base/validation.h"
#include "renderer/backend/vulkan/device_holder_vk.h"

namespace ogre {

YUVConversionLibraryVK::YUVConversionLibraryVK(
    std::weak_ptr<DeviceHolderVK> device_holder)
    : device_holder_(std::move(device_holder)) {}

YUVConversionLibraryVK::~YUVConversionLibraryVK() = default;

std::shared_ptr<YUVConversionVK> YUVConversionLibraryVK::GetConversion(
    const YUVConversionDescriptorVK& desc) {
  Lock lock(conversions_mutex_);
  auto found = conversions_.find(desc);
  if (found != conversions_.end()) {
    return found->second;
  }
  auto device_holder = device_holder_.lock();
  if (!device_holder) {
    VALIDATION_LOG << "Context loss during creation of YUV conversion.";
    return nullptr;
  }
  return (conversions_[desc] = std::shared_ptr<YUVConversionVK>(
              new YUVConversionVK(device_holder->GetDevice(), desc)));
}

}  // namespace ogre

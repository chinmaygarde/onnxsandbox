// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/yuv_conversion_library_vk.h"

#include "renderer/device_holder_vk.h"

namespace ogre {

YUVConversionLibrary::YUVConversionLibrary(
    std::weak_ptr<DeviceHolder> device_holder)
    : device_holder_(std::move(device_holder)) {}

YUVConversionLibrary::~YUVConversionLibrary() = default;

std::shared_ptr<YUVConversion> YUVConversionLibrary::GetConversion(
    const YUVConversionDescriptor& desc) {
  Lock lock(conversions_mutex_);
  auto found = conversions_.find(desc);
  if (found != conversions_.end()) {
    return found->second;
  }
  auto device_holder = device_holder_.lock();
  if (!device_holder) {
    LOG(ERROR) << "Context loss during creation of YUV conversion.";
    return nullptr;
  }
  return (conversions_[desc] = std::shared_ptr<YUVConversion>(
              new YUVConversion(device_holder->GetDevice(), desc)));
}

}  // namespace ogre

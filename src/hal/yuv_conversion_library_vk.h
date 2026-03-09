// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "hal/yuv_conversion_vk.h"

namespace ogre {

class DeviceHolder;

//------------------------------------------------------------------------------
/// @brief      Due the way the Vulkan spec. treats "identically defined"
///             conversions, creating two conversion with identical descriptors,
///             using one with the image and the other with the sampler, is
///             invalid use.
///
///             A conversion library hashes and caches identical descriptors to
///             de-duplicate conversions.
///
///             There can only be one conversion library (the constructor is
///             private to force this) and it found in the context.
///
class YUVConversionLibrary {
 public:
  ~YUVConversionLibrary();

  YUVConversionLibrary(const YUVConversionLibrary&) = delete;

  YUVConversionLibrary& operator=(const YUVConversionLibrary&) = delete;

  //----------------------------------------------------------------------------
  /// @brief      Get a conversion for the given descriptor. If there is already
  ///             a conversion created for an equivalent descriptor, a reference
  ///             to that descriptor is returned instead.
  ///
  /// @param[in]  desc  The descriptor.
  ///
  /// @return     The conversion. A previously created conversion if one was
  ///             present and a new one if not. A newly created conversion is
  ///             cached for subsequent accesses.
  ///
  std::shared_ptr<YUVConversion> GetConversion(
      const YUVConversionDescriptor& chain);

 private:
  friend class Context;

  using ConversionsMap = std::unordered_map<YUVConversionDescriptor,
                                            std::shared_ptr<YUVConversion>,
                                            YUVConversionDescriptorHash,
                                            YUVConversionDescriptorEqual>;

  std::weak_ptr<DeviceHolder> device_holder_;
  Mutex conversions_mutex_;
  ConversionsMap conversions_ IPLR_GUARDED_BY(conversions_mutex_);

  explicit YUVConversionLibrary(std::weak_ptr<DeviceHolder> device_holder);
};

}  // namespace ogre

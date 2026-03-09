// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "base/comparable.h"
#include "base/thread.h"
#include "core/sampler_descriptor.h"
#include "fml/build_config.h"
#include "hal/sampler_vk.h"
#include "hal/shared_object_vk.h"
#include "hal/vk.h"

namespace ogre {

//------------------------------------------------------------------------------
/// A descriptor used to create a new YUV conversion in a conversion library.
///
using YUVConversionDescriptor =
    vk::StructureChain<vk::SamplerYcbcrConversionCreateInfo
#if FML_OS_ANDROID
                       // For VK_ANDROID_external_memory_android_hardware_buffer
                       ,
                       vk::ExternalFormatANDROID
#endif  // FML_OS_ANDROID
                       >;

class YUVConversionLibrary;

//------------------------------------------------------------------------------
/// @brief      It is sometimes necessary to deal with formats not native to
///             Vulkan. In such cases, extra information is necessary to access
///             images. A YUV conversion object is needed in such instances.
///
///             There are usually only a handful of viable conversions in a
///             given context. However, due to the way the Vulkan spec. treats
///             "identically defined" conversions, only a single conversion
///             object is valid for an equivalent `YUVConversionDescriptor`.
///             Because of this restriction, it is not possible to just create a
///             conversion from a descriptor (as the underlying handles will be
///             equivalent but different). Instead, a conversion may only be
///             obtained from a conversion library. Libraries handle hashing and
///             caching conversions by descriptor. Caller can find a library on
///             the top-level context. They may not create their own (the
///             constructor is private).
///
class YUVConversion final {
 public:
  ~YUVConversion();

  YUVConversion(const YUVConversion&) = delete;

  YUVConversion& operator=(const YUVConversion&) = delete;

  //----------------------------------------------------------------------------
  /// @return     `true` if this conversion is valid for use with images and
  ///             samplers.
  ///
  bool IsValid() const;

  //----------------------------------------------------------------------------
  /// @brief      Get the descriptor used to create this conversion.
  ///
  const YUVConversionDescriptor& GetDescriptor() const;

  //----------------------------------------------------------------------------
  /// @return     The Vulkan handle of the YUV conversion.
  ///
  vk::SamplerYcbcrConversion GetConversion() const;

 private:
  friend class YUVConversionLibrary;

  YUVConversionDescriptor chain_;
  vk::UniqueSamplerYcbcrConversion conversion_;

  YUVConversion(const vk::Device& device, const YUVConversionDescriptor& chain);
};

struct YUVConversionDescriptorHash {
  std::size_t operator()(const YUVConversionDescriptor& object) const;
};

struct YUVConversionDescriptorEqual {
  bool operator()(const YUVConversionDescriptor& lhs,
                  const YUVConversionDescriptor& rhs) const;
};

struct ImmutableSamplerKey : public Comparable<ImmutableSamplerKey> {
  SamplerDescriptor sampler;
  YUVConversionDescriptor yuv_conversion;

  explicit ImmutableSamplerKey(const Sampler& sampler);

  // |Comparable<ImmutableSamplerKey>|
  std::size_t GetHash() const override;

  // |Comparable<ImmutableSamplerKey>|
  bool IsEqual(const ImmutableSamplerKey& other) const override;
};

}  // namespace ogre

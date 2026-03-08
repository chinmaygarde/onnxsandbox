// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "renderer/backend/vulkan/texture_source_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/backend/vulkan/yuv_conversion_vk.h"
#include "toolkit/android/hardware_buffer.h"

#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>

namespace ogre {

//------------------------------------------------------------------------------
/// @brief      A texture source that wraps an instance of AHardwareBuffer.
///
///             The formats and conversions supported by Android Hardware
///             Buffers are a superset of those supported by Impeller (and
///             Vulkan for that matter). Impeller and Vulkan descriptors
///             obtained from the these texture sources are advisory and it
///             usually isn't possible to create copies of images and image
///             views held in these texture sources using the inferred
///             descriptors. The objects are meant to be used directly (either
///             as render targets or sources for sampling), not copied.
///
class AHBTextureSource final : public TextureSource {
 public:
  AHBTextureSource(const std::shared_ptr<Context>& context,
                   struct AHardwareBuffer* hardware_buffer,
                   const AHardwareBuffer_Desc& hardware_buffer_desc);

  AHBTextureSource(const std::shared_ptr<Context>& context,
                   std::unique_ptr<android::HardwareBuffer> backing_store,
                   bool is_swapchain_image);

  // |TextureSource|
  ~AHBTextureSource() override;

  // |TextureSource|
  vk::Image GetImage() const override;

  // |TextureSource|
  vk::ImageView GetImageView() const override;

  // |TextureSource|
  vk::ImageView GetRenderTargetView() const override;

  bool IsValid() const;

  // |TextureSource|
  bool IsSwapchainImage() const override;

  // |TextureSource|
  std::shared_ptr<YUVConversion> GetYUVConversion() const override;

  const android::HardwareBuffer* GetBackingStore() const;

  using AHBProperties = vk::StructureChain<
      // For VK_ANDROID_external_memory_android_hardware_buffer
      vk::AndroidHardwareBufferPropertiesANDROID,
      // For VK_ANDROID_external_memory_android_hardware_buffer
      vk::AndroidHardwareBufferFormatPropertiesANDROID>;

  using ImageViewInfo = vk::StructureChain<vk::ImageViewCreateInfo,
                                           // Core in 1.1
                                           vk::SamplerYcbcrConversionInfo>;

  /// Create a VkImage that wraps an Android hardware buffer.
  static vk::UniqueImage CreateVKImageWrapperForAndroidHarwareBuffer(
      const vk::Device& device,
      const AHBProperties& ahb_props,
      const AHardwareBuffer_Desc& ahb_desc);

  /// Create a VkImageViewCreateInfo that matches the properties of an Android
  /// hardware buffer.
  static ImageViewInfo CreateImageViewInfo(
      const vk::Image& image,
      const std::shared_ptr<YUVConversion>& yuv_conversion_wrapper,
      const AHBProperties& ahb_props,
      const AHardwareBuffer_Desc& ahb_desc);

 private:
  std::unique_ptr<android::HardwareBuffer> backing_store_;
  vk::UniqueDeviceMemory device_memory_ = {};
  vk::UniqueImage image_ = {};
  vk::UniqueImageView image_view_ = {};
  std::shared_ptr<YUVConversion> yuv_conversion_ = {};
  bool needs_yuv_conversion_ = false;
  bool is_swapchain_image_ = false;
  bool is_valid_ = false;

  AHBTextureSource(const AHBTextureSource&) = delete;

  AHBTextureSource& operator=(const AHBTextureSource&) = delete;
};

}  // namespace ogre

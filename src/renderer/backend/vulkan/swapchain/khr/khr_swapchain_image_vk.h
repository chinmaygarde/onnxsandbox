// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_IMAGE_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_IMAGE_VK_H_

#include "geometry/size.h"
#include "renderer/backend/vulkan/formats_vk.h"
#include "renderer/backend/vulkan/texture_source_vk.h"
#include "renderer/backend/vulkan/vk.h"

namespace ogre {

class KHRSwapchainImage final : public TextureSource {
 public:
  KHRSwapchainImage(TextureDescriptor desc,
                    const vk::Device& device,
                    vk::Image image);

  // |TextureSource|
  ~KHRSwapchainImage() override;

  bool IsValid() const;

  // |TextureSource|
  vk::Image GetImage() const override;

  // |TextureSource|
  vk::ImageView GetImageView() const override;

  // |TextureSource|
  vk::ImageView GetRenderTargetView() const override;

  // |TextureSource|
  bool IsSwapchainImage() const override;

 private:
  vk::Image image_ = VK_NULL_HANDLE;
  vk::UniqueImageView image_view_ = {};
  bool is_valid_ = false;

  KHRSwapchainImage(const KHRSwapchainImage&) = delete;

  KHRSwapchainImage& operator=(const KHRSwapchainImage&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_KHR_KHR_SWAPCHAIN_IMAGE_VK_H_

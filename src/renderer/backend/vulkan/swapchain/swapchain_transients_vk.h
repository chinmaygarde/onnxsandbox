// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_TRANSIENTS_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_TRANSIENTS_VK_H_

#include "core/texture.h"
#include "core/texture_descriptor.h"

namespace ogre {

class Context;

//------------------------------------------------------------------------------
/// @brief      Resources, meant to be memoized by the texture descriptor of the
///             wrapped swapchain images, that are intuitively cheap to create
///             but have been observed to be time consuming to construct on some
///             Vulkan drivers. This includes the device-transient MSAA and
///             depth-stencil textures.
///
///             The same textures are used for all swapchain images.
///
class SwapchainTransients {
 public:
  explicit SwapchainTransients(std::weak_ptr<Context> context,
                               TextureDescriptor desc,
                               bool enable_msaa);

  ~SwapchainTransients();

  SwapchainTransients(const SwapchainTransients&) = delete;

  SwapchainTransients& operator=(const SwapchainTransients&) = delete;

  const std::weak_ptr<Context>& GetContext() const;

  bool IsMSAAEnabled() const;

  const std::shared_ptr<Texture>& GetMSAATexture();

  const std::shared_ptr<Texture>& GetDepthStencilTexture();

 private:
  std::weak_ptr<Context> context_;
  const TextureDescriptor desc_;
  const bool enable_msaa_;
  std::shared_ptr<Texture> cached_msaa_texture_;
  std::shared_ptr<Texture> cached_depth_stencil_;

  std::shared_ptr<Texture> CreateMSAATexture() const;

  std::shared_ptr<Texture> CreateDepthStencilTexture() const;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_SWAPCHAIN_TRANSIENTS_VK_H_

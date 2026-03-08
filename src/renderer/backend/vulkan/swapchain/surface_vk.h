// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "geometry/size.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/swapchain/swapchain_transients_vk.h"
#include "renderer/backend/vulkan/texture_source_vk.h"
#include "renderer/render_target.h"

namespace ogre {

class Surface final {
 public:
  using SwapCallback = std::function<bool(void)>;

  /// @brief Wrap the swapchain image in a Surface, which provides the
  ///        additional configuration required for usage as on onscreen render
  ///        target by Impeller.
  ///
  ///        This creates the associated MSAA and depth+stencil texture.
  static std::unique_ptr<Surface> WrapSwapchainImage(
      const std::shared_ptr<SwapchainTransients>& transients,
      const std::shared_ptr<TextureSource>& swapchain_image,
      SwapCallback swap_callback);

  ~Surface();

  const ISize& GetSize() const;

  bool IsValid() const;

  const RenderTarget& GetRenderTarget() const;

  bool Present() const;

 private:
  RenderTarget desc_;
  ISize size_;
  bool is_valid_ = false;
  SwapCallback swap_callback_;

  Surface(const RenderTarget& target, SwapCallback swap_callback);

  Surface(const Surface&) = delete;

  Surface& operator=(const Surface&) = delete;
};

}  // namespace ogre

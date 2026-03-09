// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/swapchain/surface_vk.h"

#include "core/formats.h"
#include "hal/texture_vk.h"

namespace ogre {

std::unique_ptr<Surface> Surface::WrapSwapchainImage(
    const std::shared_ptr<SwapchainTransients>& transients,
    const std::shared_ptr<TextureSource>& swapchain_image,
    SwapCallback swap_callback) {
  if (!transients || !swapchain_image || !swap_callback) {
    return nullptr;
  }

  auto context = transients->GetContext().lock();
  if (!context) {
    return nullptr;
  }

  const auto enable_msaa = transients->IsMSAAEnabled();

  const auto swapchain_tex_desc = swapchain_image->GetTextureDescriptor();

  TextureDescriptor resolve_tex_desc;
  resolve_tex_desc.type = TextureType::kTexture2D;
  resolve_tex_desc.format = swapchain_tex_desc.format;
  resolve_tex_desc.size = swapchain_tex_desc.size;
  resolve_tex_desc.usage = TextureUsage::kRenderTarget;
  resolve_tex_desc.sample_count = SampleCount::kCount1;
  resolve_tex_desc.storage_mode = StorageMode::kDevicePrivate;

  std::shared_ptr<TextureVK> resolve_tex =
      std::make_shared<TextureVK>(context,         //
                                  swapchain_image  //
      );

  if (!resolve_tex) {
    return nullptr;
  }
  resolve_tex->SetLabel("ImpellerOnscreenResolve");

  ColorAttachment color0;
  color0.clear_color = Color::DarkSlateGray();
  color0.load_action = LoadAction::kClear;
  if (enable_msaa) {
    color0.texture = transients->GetMSAATexture();
    color0.store_action = StoreAction::kMultisampleResolve;
    color0.resolve_texture = resolve_tex;
  } else {
    color0.texture = resolve_tex;
    color0.store_action = StoreAction::kStore;
  }

  RenderTarget render_target_desc;
  render_target_desc.SetColorAttachment(color0, 0u);
  render_target_desc.SetupDepthStencilAttachments(
      /*context=*/*context,                            //
      /*allocator=*/*context->GetResourceAllocator(),  //
      /*size=*/swapchain_tex_desc.size,                //
      /*msaa=*/enable_msaa,                            //
      /*label=*/"Onscreen",                            //
      /*stencil_attachment_config=*/
      RenderTarget::kDefaultStencilAttachmentConfig,                  //
      /*depth_stencil_texture=*/transients->GetDepthStencilTexture()  //
  );

  // The constructor is private. So make_unique may not be used.
  return std::unique_ptr<Surface>(
      new Surface(render_target_desc, std::move(swap_callback)));
}

Surface::Surface(const RenderTarget& target, SwapCallback swap_callback)
    : desc_(target), swap_callback_(std::move(swap_callback)) {
  if (auto size = desc_.GetColorAttachmentSize(0u); size.has_value()) {
    size_ = size.value();
  } else {
    return;
  }
  is_valid_ = true;
}

Surface::~Surface() = default;

const ISize& Surface::GetSize() const {
  return size_;
}

bool Surface::IsValid() const {
  return is_valid_;
}

const RenderTarget& Surface::GetRenderTarget() const {
  return desc_;
}

bool Surface::Present() const {
  return swap_callback_ ? swap_callback_() : false;
}

}  // namespace ogre

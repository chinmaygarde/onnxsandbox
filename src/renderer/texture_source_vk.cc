// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/texture_source_vk.h"

namespace ogre {

TextureSource::TextureSource(TextureDescriptor desc) : desc_(desc) {}

TextureSource::~TextureSource() = default;

const TextureDescriptor& TextureSource::GetTextureDescriptor() const {
  return desc_;
}

std::shared_ptr<YUVConversion> TextureSource::GetYUVConversion() const {
  return nullptr;
}

vk::ImageLayout TextureSource::GetLayout() const {
  return layout_;
}

vk::ImageLayout TextureSource::SetLayoutWithoutEncoding(
    vk::ImageLayout layout) const {
  const auto old_layout = layout_;
  layout_ = layout;
  return old_layout;
}

fml::Status TextureSource::SetLayout(const Barrier& barrier) const {
  const vk::ImageLayout old_layout =
      SetLayoutWithoutEncoding(barrier.new_layout);
  vk::ImageMemoryBarrier image_barrier;
  image_barrier.srcAccessMask = barrier.src_access;
  image_barrier.dstAccessMask = barrier.dst_access;
  image_barrier.oldLayout = old_layout;
  image_barrier.newLayout = barrier.new_layout;
  image_barrier.image = GetImage();
  image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.subresourceRange.aspectMask = ToImageAspectFlags(desc_.format);
  image_barrier.subresourceRange.baseMipLevel = barrier.base_mip_level;
  image_barrier.subresourceRange.levelCount =
      desc_.mip_count - barrier.base_mip_level;
  image_barrier.subresourceRange.baseArrayLayer = 0u;
  image_barrier.subresourceRange.layerCount = ToArrayLayerCount(desc_.type);

  barrier.cmd_buffer.pipelineBarrier(barrier.src_stage,  // src stage
                                     barrier.dst_stage,  // dst stage
                                     {},                 // dependency flags
                                     nullptr,            // memory barriers
                                     nullptr,            // buffer barriers
                                     image_barrier       // image barriers
  );

  return {};
}

void TextureSource::SetCachedFrameData(const FramebufferAndRenderPass& data,
                                       SampleCount sample_count) {
  frame_data_[static_cast<int>(sample_count) / 4] = data;
}

const FramebufferAndRenderPass& TextureSource::GetCachedFrameData(
    SampleCount sample_count) const {
  return frame_data_[static_cast<int>(sample_count) / 4];
}

}  // namespace ogre

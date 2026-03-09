// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/blit_pass_vk.h"

#include <format>

#include "core/formats.h"
#include "renderer/barrier_vk.h"
#include "renderer/command_buffer_vk.h"
#include "renderer/texture_vk.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

namespace ogre {

static void InsertImageMemoryBarrier(const vk::CommandBuffer& cmd,
                                     const vk::Image& image,
                                     vk::AccessFlags src_access_mask,
                                     vk::AccessFlags dst_access_mask,
                                     vk::ImageLayout old_layout,
                                     vk::ImageLayout new_layout,
                                     vk::PipelineStageFlags src_stage,
                                     vk::PipelineStageFlags dst_stage,
                                     uint32_t base_mip_level,
                                     uint32_t mip_level_count = 1u) {
  if (old_layout == new_layout) {
    return;
  }

  vk::ImageMemoryBarrier barrier;
  barrier.srcAccessMask = src_access_mask;
  barrier.dstAccessMask = dst_access_mask;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = base_mip_level;
  barrier.subresourceRange.levelCount = mip_level_count;
  barrier.subresourceRange.baseArrayLayer = 0u;
  barrier.subresourceRange.layerCount = 1u;

  cmd.pipelineBarrier(src_stage, dst_stage, {}, nullptr, nullptr, barrier);
}

BlitPass::BlitPass(std::shared_ptr<CommandBuffer> command_buffer,
                   const Workarounds& workarounds)
    : command_buffer_(std::move(command_buffer)), workarounds_(workarounds) {}

BlitPass::~BlitPass() = default;

void BlitPass::OnSetLabel(std::string_view label) {}

void BlitPass::SetLabel(std::string_view label) {
  if (label.empty()) {
    return;
  }
  OnSetLabel(label);
}

bool BlitPass::AddCopy(std::shared_ptr<TextureVK> source,
                       std::shared_ptr<TextureVK> destination,
                       std::optional<IRect> source_region,
                       IPoint destination_origin,
                       std::string_view label) {
  if (!source) {
    LOG(ERROR) << "Attempted to add a texture blit with no source.";
    return false;
  }
  if (!destination) {
    LOG(ERROR) << "Attempted to add a texture blit with no destination.";
    return false;
  }

  if (source->GetTextureDescriptor().sample_count !=
      destination->GetTextureDescriptor().sample_count) {
    LOG(ERROR) << std::format(
        "The source sample count ({}) must match the destination sample count "
        "({}) for blits.",
        static_cast<int>(source->GetTextureDescriptor().sample_count),
        static_cast<int>(destination->GetTextureDescriptor().sample_count));
    return false;
  }
  if (source->GetTextureDescriptor().format !=
      destination->GetTextureDescriptor().format) {
    LOG(ERROR) << std::format(
        "The source pixel format ({}) must match the destination pixel format "
        "({}) "
        "for blits.",
        PixelFormatToString(source->GetTextureDescriptor().format),
        PixelFormatToString(destination->GetTextureDescriptor().format));
    return false;
  }

  if (!source_region.has_value()) {
    source_region = IRect::MakeSize(source->GetSize());
  }

  source_region =
      source_region->Intersection(IRect::MakeSize(source->GetSize()));
  if (!source_region.has_value()) {
    return true;  // Nothing to blit.
  }

  return OnCopyTextureToTextureCommand(
      std::move(source), std::move(destination), source_region.value(),
      destination_origin, label);
}

bool BlitPass::AddCopy(std::shared_ptr<TextureVK> source,
                       std::shared_ptr<DeviceBufferVK> destination,
                       std::optional<IRect> source_region,
                       size_t destination_offset,
                       std::string_view label) {
  if (!source) {
    LOG(ERROR) << "Attempted to add a texture blit with no source.";
    return false;
  }
  if (!destination) {
    LOG(ERROR) << "Attempted to add a texture blit with no destination.";
    return false;
  }

  if (!source_region.has_value()) {
    source_region = IRect::MakeSize(source->GetSize());
  }

  auto bytes_per_pixel =
      BytesPerPixelForPixelFormat(source->GetTextureDescriptor().format);
  auto bytes_per_image = source_region->Area() * bytes_per_pixel;
  if (destination_offset + bytes_per_image >
      destination->GetDeviceBufferDescriptor().size) {
    LOG(ERROR) << "Attempted to add a texture blit with out of bounds access.";
    return false;
  }

  source_region =
      source_region->Intersection(IRect::MakeSize(source->GetSize()));
  if (!source_region.has_value()) {
    return true;  // Nothing to blit.
  }

  return OnCopyTextureToBufferCommand(std::move(source), std::move(destination),
                                      source_region.value(), destination_offset,
                                      label);
}

bool BlitPass::AddCopy(BufferView source,
                       std::shared_ptr<TextureVK> destination,
                       std::optional<IRect> destination_region,
                       std::string_view label,
                       uint32_t mip_level,
                       uint32_t slice,
                       bool convert_to_read) {
  if (!destination) {
    LOG(ERROR) << "Attempted to add a texture blit with no destination.";
    return false;
  }
  ISize destination_size = destination->GetSize();
  IRect destination_region_value =
      destination_region.value_or(IRect::MakeSize(destination_size));
  if (destination_region_value.GetX() < 0 ||
      destination_region_value.GetY() < 0 ||
      destination_region_value.GetRight() > destination_size.width ||
      destination_region_value.GetBottom() > destination_size.height) {
    LOG(ERROR) << "Blit region cannot be larger than destination texture.";
    return false;
  }

  auto bytes_per_pixel =
      BytesPerPixelForPixelFormat(destination->GetTextureDescriptor().format);
  auto bytes_per_region = destination_region_value.Area() * bytes_per_pixel;

  if (source.GetRange().length != bytes_per_region) {
    LOG(ERROR) << "Attempted to add a texture blit with out of bounds access.";
    return false;
  }
  if (mip_level >= destination->GetMipCount()) {
    LOG(ERROR) << "Invalid value for mip_level: " << mip_level << ". "
               << "The destination texture has " << destination->GetMipCount()
               << " mip levels.";
    return false;
  }
  if (slice > 5) {
    LOG(ERROR) << "Invalid value for slice: " << slice;
    return false;
  }

  return OnCopyBufferToTextureCommand(std::move(source), std::move(destination),
                                      destination_region_value, label,
                                      mip_level, slice, convert_to_read);
}

bool BlitPass::GenerateMipmap(std::shared_ptr<TextureVK> texture,
                              std::string_view label) {
  if (!texture) {
    LOG(ERROR) << "Attempted to add an invalid mipmap generation command "
                  "with no texture.";
    return false;
  }
  return OnGenerateMipmapCommand(std::move(texture), label);
}

bool BlitPass::IsValid() const {
  return true;
}

bool BlitPass::EncodeCommands() const {
  return true;
}

bool BlitPass::OnCopyTextureToTextureCommand(
    std::shared_ptr<TextureVK> source,
    std::shared_ptr<TextureVK> destination,
    IRect source_region,
    IPoint destination_origin,
    std::string_view label) {
  const auto& cmd_buffer = command_buffer_->GetCommandBuffer();

  const auto& src = *source;
  const auto& dst = *destination;

  if (!command_buffer_->Track(source) || !command_buffer_->Track(destination)) {
    return false;
  }

  Barrier src_barrier;
  src_barrier.cmd_buffer = cmd_buffer;
  src_barrier.new_layout = vk::ImageLayout::eTransferSrcOptimal;
  src_barrier.src_access = vk::AccessFlagBits::eTransferWrite |
                           vk::AccessFlagBits::eShaderWrite |
                           vk::AccessFlagBits::eColorAttachmentWrite;
  src_barrier.src_stage = vk::PipelineStageFlagBits::eTransfer |
                          vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eColorAttachmentOutput;
  src_barrier.dst_access = vk::AccessFlagBits::eTransferRead;
  src_barrier.dst_stage = vk::PipelineStageFlagBits::eTransfer;

  Barrier dst_barrier;
  dst_barrier.cmd_buffer = cmd_buffer;
  dst_barrier.new_layout = vk::ImageLayout::eTransferDstOptimal;
  dst_barrier.src_access = {};
  dst_barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  dst_barrier.dst_access =
      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferWrite;
  dst_barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eTransfer;

  if (!src.SetLayout(src_barrier) || !dst.SetLayout(dst_barrier)) {
    LOG(ERROR) << "Could not complete layout transitions.";
    return false;
  }

  vk::ImageCopy image_copy;

  image_copy.setSrcSubresource(
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1));
  image_copy.setDstSubresource(
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1));

  image_copy.srcOffset =
      vk::Offset3D(source_region.GetX(), source_region.GetY(), 0);
  image_copy.dstOffset =
      vk::Offset3D(destination_origin.x, destination_origin.y, 0);
  image_copy.extent =
      vk::Extent3D(source_region.GetWidth(), source_region.GetHeight(), 1);

  // Issue the copy command now that the images are already in the right
  // layouts.
  cmd_buffer.copyImage(src.GetImage(),          //
                       src_barrier.new_layout,  //
                       dst.GetImage(),          //
                       dst_barrier.new_layout,  //
                       image_copy               //
  );

  // If this is an onscreen texture, do not transition the layout
  // back to shader read.
  if (dst.IsSwapchainImage()) {
    return true;
  }

  Barrier barrier;
  barrier.cmd_buffer = cmd_buffer;
  barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.src_access = {};
  barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  barrier.dst_access = vk::AccessFlagBits::eShaderRead;
  barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

  return dst.SetLayout(barrier);
}

bool BlitPass::OnCopyTextureToBufferCommand(
    std::shared_ptr<TextureVK> source,
    std::shared_ptr<DeviceBufferVK> destination,
    IRect source_region,
    size_t destination_offset,
    std::string_view label) {
  const auto& cmd_buffer = command_buffer_->GetCommandBuffer();

  const auto& src = *source;

  if (!command_buffer_->Track(source) || !command_buffer_->Track(destination)) {
    return false;
  }

  Barrier barrier;
  barrier.cmd_buffer = cmd_buffer;
  barrier.new_layout = vk::ImageLayout::eTransferSrcOptimal;
  barrier.src_access = vk::AccessFlagBits::eShaderWrite |
                       vk::AccessFlagBits::eTransferWrite |
                       vk::AccessFlagBits::eColorAttachmentWrite;
  barrier.src_stage = vk::PipelineStageFlagBits::eFragmentShader |
                      vk::PipelineStageFlagBits::eTransfer |
                      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  barrier.dst_access = vk::AccessFlagBits::eShaderRead;
  barrier.dst_stage = vk::PipelineStageFlagBits::eVertexShader |
                      vk::PipelineStageFlagBits::eFragmentShader;

  const DeviceBufferVK& dst = *destination;

  vk::BufferImageCopy image_copy;
  image_copy.setBufferOffset(destination_offset);
  image_copy.setBufferRowLength(0);
  image_copy.setBufferImageHeight(0);
  image_copy.setImageSubresource(
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1));
  image_copy.setImageOffset(
      vk::Offset3D(source_region.GetX(), source_region.GetY(), 0));
  image_copy.setImageExtent(
      vk::Extent3D(source_region.GetWidth(), source_region.GetHeight(), 1));

  if (!src.SetLayout(barrier)) {
    LOG(ERROR) << "Could not encode layout transition.";
    return false;
  }

  cmd_buffer.copyImageToBuffer(src.GetImage(),      //
                               barrier.new_layout,  //
                               dst.GetBuffer(),     //
                               image_copy           //
  );

  // If the buffer is used for readback, then apply a transfer -> host memory
  // barrier.
  if (destination->GetDeviceBufferDescriptor().readback) {
    vk::MemoryBarrier barrier;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eHostRead;

    cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eHost, {}, 1,
                               &barrier, 0, {}, 0, {});
  }

  return true;
}

bool BlitPass::ConvertTextureToShaderRead(
    const std::shared_ptr<TextureVK>& texture) {
  const auto& cmd_buffer = command_buffer_->GetCommandBuffer();

  Barrier barrier;
  barrier.cmd_buffer = cmd_buffer;
  barrier.src_access = vk::AccessFlagBits::eTransferWrite;
  barrier.src_stage = vk::PipelineStageFlagBits::eTransfer;
  barrier.dst_access = vk::AccessFlagBits::eShaderRead;
  barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

  barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

  if (!command_buffer_->Track(texture)) {
    return false;
  }

  return texture->SetLayout(barrier);
}

bool BlitPass::OnCopyBufferToTextureCommand(
    BufferView source,
    std::shared_ptr<TextureVK> destination,
    IRect destination_region,
    std::string_view label,
    uint32_t mip_level,
    uint32_t slice,
    bool convert_to_read) {
  const auto& cmd_buffer = command_buffer_->GetCommandBuffer();

  const auto& dst = *destination;
  const DeviceBufferVK& src = *source.GetBuffer();

  std::shared_ptr<const DeviceBufferVK> source_buffer = source.TakeBuffer();
  if ((source_buffer && !command_buffer_->Track(source_buffer)) ||
      !command_buffer_->Track(destination)) {
    return false;
  }

  Barrier dst_barrier;
  dst_barrier.cmd_buffer = cmd_buffer;
  dst_barrier.new_layout = vk::ImageLayout::eTransferDstOptimal;
  dst_barrier.src_access = {};
  dst_barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  dst_barrier.dst_access =
      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferWrite;
  dst_barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eTransfer;

  vk::BufferImageCopy image_copy;
  image_copy.setBufferOffset(source.GetRange().offset);
  image_copy.setBufferRowLength(0);
  image_copy.setBufferImageHeight(0);
  image_copy.setImageSubresource(vk::ImageSubresourceLayers(
      vk::ImageAspectFlagBits::eColor, mip_level, slice, 1));
  image_copy.imageOffset.x = destination_region.GetX();
  image_copy.imageOffset.y = destination_region.GetY();
  image_copy.imageOffset.z = 0u;
  image_copy.imageExtent.width = destination_region.GetWidth();
  image_copy.imageExtent.height = destination_region.GetHeight();
  image_copy.imageExtent.depth = 1u;

  // Note: this barrier should do nothing if we're already in the transfer dst
  // optimal state. This is important for performance of repeated blit pass
  // encoding.
  if (!dst.SetLayout(dst_barrier)) {
    LOG(ERROR) << "Could not encode layout transition.";
    return false;
  }

  cmd_buffer.copyBufferToImage(src.GetBuffer(),         //
                               dst.GetImage(),          //
                               dst_barrier.new_layout,  //
                               image_copy               //
  );

  // Transition to shader-read.
  if (convert_to_read) {
    Barrier barrier;
    barrier.cmd_buffer = cmd_buffer;
    barrier.src_access = vk::AccessFlagBits::eTransferWrite;
    barrier.src_stage = vk::PipelineStageFlagBits::eTransfer;
    barrier.dst_access = vk::AccessFlagBits::eShaderRead;
    barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

    barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

    if (!dst.SetLayout(barrier)) {
      return false;
    }
  }

  return true;
}

bool BlitPass::ResizeTexture(const std::shared_ptr<TextureVK>& source,
                             const std::shared_ptr<TextureVK>& destination) {
  const auto& cmd_buffer = command_buffer_->GetCommandBuffer();

  const auto& src = *source;
  const auto& dst = *destination;

  if (!command_buffer_->Track(source) || !command_buffer_->Track(destination)) {
    return false;
  }

  Barrier src_barrier;
  src_barrier.cmd_buffer = cmd_buffer;
  src_barrier.new_layout = vk::ImageLayout::eTransferSrcOptimal;
  src_barrier.src_access = vk::AccessFlagBits::eTransferWrite |
                           vk::AccessFlagBits::eShaderWrite |
                           vk::AccessFlagBits::eColorAttachmentWrite;
  src_barrier.src_stage = vk::PipelineStageFlagBits::eTransfer |
                          vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eColorAttachmentOutput;
  src_barrier.dst_access = vk::AccessFlagBits::eTransferRead;
  src_barrier.dst_stage = vk::PipelineStageFlagBits::eTransfer;

  Barrier dst_barrier;
  dst_barrier.cmd_buffer = cmd_buffer;
  dst_barrier.new_layout = vk::ImageLayout::eTransferDstOptimal;
  dst_barrier.src_access = {};
  dst_barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  dst_barrier.dst_access =
      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferWrite;
  dst_barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eTransfer;

  if (!src.SetLayout(src_barrier) || !dst.SetLayout(dst_barrier)) {
    LOG(ERROR) << "Could not complete layout transitions.";
    return false;
  }

  vk::ImageBlit blit;
  blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  blit.srcSubresource.baseArrayLayer = 0u;
  blit.srcSubresource.layerCount = 1u;
  blit.srcSubresource.mipLevel = 0;

  blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  blit.dstSubresource.baseArrayLayer = 0u;
  blit.dstSubresource.layerCount = 1u;
  blit.dstSubresource.mipLevel = 0;

  // offsets[0] is origin.
  blit.srcOffsets[1].x = std::max<int32_t>(source->GetSize().width, 1u);
  blit.srcOffsets[1].y = std::max<int32_t>(source->GetSize().height, 1u);
  blit.srcOffsets[1].z = 1u;

  // offsets[0] is origin.
  blit.dstOffsets[1].x = std::max<int32_t>(destination->GetSize().width, 1u);
  blit.dstOffsets[1].y = std::max<int32_t>(destination->GetSize().height, 1u);
  blit.dstOffsets[1].z = 1u;

  cmd_buffer.blitImage(src.GetImage(),          //
                       src_barrier.new_layout,  //
                       dst.GetImage(),          //
                       dst_barrier.new_layout,  //
                       1,                       //
                       &blit,                   //
                       vk::Filter::eLinear

  );

  // Convert back to shader read

  Barrier barrier;
  barrier.cmd_buffer = cmd_buffer;
  barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.src_access = {};
  barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  barrier.dst_access = vk::AccessFlagBits::eShaderRead;
  barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

  return dst.SetLayout(barrier);
}

bool BlitPass::OnGenerateMipmapCommand(std::shared_ptr<TextureVK> texture,
                                       std::string_view label) {
  auto& src = *texture;

  const auto size = src.GetTextureDescriptor().size;
  uint32_t mip_count = src.GetTextureDescriptor().mip_count;

  if (mip_count < 2u) {
    return true;
  }

  const auto& image = src.GetImage();
  const auto& cmd = command_buffer_->GetCommandBuffer();

  if (!command_buffer_->Track(texture)) {
    return false;
  }

  // Initialize all mip levels to be in TransferDst mode. Later, in a loop,
  // after writing to that mip level, we'll first switch its layout to
  // TransferSrc to prepare the mip level after it, use the image as the source
  // of the blit, before finally switching it to ShaderReadOnly so its available
  // for sampling in a shader.
  InsertImageMemoryBarrier(
      /*cmd=*/cmd,
      /*image=*/image,
      /*src_access_mask=*/vk::AccessFlagBits::eTransferWrite |
          vk::AccessFlagBits::eColorAttachmentWrite,
      /*dst_access_mask=*/vk::AccessFlagBits::eTransferRead,
      /*old_layout=*/src.GetLayout(),
      /*new_layout=*/vk::ImageLayout::eTransferDstOptimal,
      /*src_stage=*/vk::PipelineStageFlagBits::eTransfer |
          vk::PipelineStageFlagBits::eColorAttachmentOutput,
      /*dst_stage=*/vk::PipelineStageFlagBits::eTransfer,
      /*base_mip_level=*/0u,
      /*mip_level_count=*/mip_count);

  vk::ImageMemoryBarrier barrier;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  // Blit from the mip level N - 1 to mip level N.
  size_t width = size.width;
  size_t height = size.height;
  for (size_t mip_level = 1u; mip_level < mip_count; mip_level++) {
    barrier.subresourceRange.baseMipLevel = mip_level - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    // We just finished writing to the previous (N-1) mip level or it was the
    // base mip level. These were initialized to TransferDst earler. We are now
    // going to read from it to write to the current level (N) . So it must be
    // converted to TransferSrc.
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                        {barrier});

    vk::ImageBlit blit;
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.baseArrayLayer = 0u;
    blit.srcSubresource.layerCount = 1u;
    blit.srcSubresource.mipLevel = mip_level - 1;

    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.baseArrayLayer = 0u;
    blit.dstSubresource.layerCount = 1u;
    blit.dstSubresource.mipLevel = mip_level;

    // offsets[0] is origin.
    blit.srcOffsets[1].x = std::max<int32_t>(width, 1u);
    blit.srcOffsets[1].y = std::max<int32_t>(height, 1u);
    blit.srcOffsets[1].z = 1u;

    width = width / 2;
    height = height / 2;
    if (width <= 1 || height <= 1) {
      // Continue to make sure everything is placed into eTransferSrcOptimal.
      continue;
    }

    // offsets[0] is origin.
    blit.dstOffsets[1].x = std::max<int32_t>(width, 1u);
    blit.dstOffsets[1].y = std::max<int32_t>(height, 1u);
    blit.dstOffsets[1].z = 1u;

    cmd.blitImage(image,                                 // src image
                  vk::ImageLayout::eTransferSrcOptimal,  // src layout
                  image,                                 // dst image
                  vk::ImageLayout::eTransferDstOptimal,  // dst layout
                  1u,                                    // region count
                  &blit,                                 // regions
                  vk::Filter::eLinear                    // filter
    );
  }

  // Switch the last one to eTransferSrcOptimal.
  barrier.subresourceRange.baseMipLevel = mip_count - 1;
  barrier.subresourceRange.levelCount = 1;
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                      {barrier});

  // Now everything is in eTransferSrcOptimal, switch everything to
  // eShaderReadOnlyOptimal.
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mip_count;
  barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                      {barrier});

  // We modified the layouts of this image from underneath it. Tell it its new
  // state so it doesn't try to perform redundant transitions under the hood.
  src.SetLayoutWithoutEncoding(vk::ImageLayout::eShaderReadOnlyOptimal);
  src.SetMipMapGenerated();

  return true;
}

}  // namespace ogre

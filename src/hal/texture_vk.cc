// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/texture_vk.h"

#include <absl/log/log.h>

#include "core/formats.h"
#include "core/texture_descriptor.h"
#include "hal/allocator_vk.h"
#include "hal/command_buffer_vk.h"
#include "hal/command_queue_vk.h"
#include "hal/formats_vk.h"
#include "hal/sampler_vk.h"

namespace ogre {

TextureVK::TextureVK(std::weak_ptr<Context> context,
                     std::shared_ptr<TextureSource> source)
    : context_(std::move(context)),
      desc_(source ? source->GetTextureDescriptor() : TextureDescriptor{}),
      source_(std::move(source)) {
#ifdef OGRE_DEBUG
  has_validation_layers_ = HasValidationLayers();
#endif  // OGRE_DEBUG
}

TextureVK::~TextureVK() = default;

void TextureVK::SetLabel(std::string_view label) {
#ifdef OGRE_DEBUG
  if (!has_validation_layers_) {
    return;
  }
  auto context = context_.lock();
  if (!context) {
    // The context may have died.
    return;
  }
  (*context).SetDebugName(GetImage(), label);
  (*context).SetDebugName(GetImageView(), label);
#endif  // OGRE_DEBUG
}

void TextureVK::SetLabel(std::string_view label, std::string_view trailing) {
#ifdef OGRE_DEBUG
  auto context = context_.lock();
  if (!context) {
    // The context may have died.
    return;
  }

  (*context).SetDebugName(GetImage(), label, trailing);
  (*context).SetDebugName(GetImageView(), label, trailing);
#endif  // OGRE_DEBUG
}

bool TextureVK::IsSliceValid(size_t slice) const {
  switch (desc_.type) {
    case TextureType::kTexture2D:
    case TextureType::kTexture2DMultisample:
    case TextureType::kTextureExternalOES:
      return slice == 0;
    case TextureType::kTextureCube:
      return slice <= 5;
  }
  LOG(FATAL) << "Reached unreachable code.";
}

bool TextureVK::SetContents(const uint8_t* contents,
                            size_t length,
                            size_t slice,
                            bool is_opaque) {
  if (!IsSliceValid(slice)) {
    LOG(ERROR) << "Invalid slice for texture.";
    return false;
  }
  if (!OnSetContents(contents, length, slice)) {
    return false;
  }
  coordinate_system_ = TextureCoordinateSystem::kUploadFromHost;
  is_opaque_ = is_opaque;
  return true;
}

bool TextureVK::SetContents(std::shared_ptr<const fml::Mapping> mapping,
                            size_t slice,
                            bool is_opaque) {
  if (!IsSliceValid(slice)) {
    LOG(ERROR) << "Invalid slice for texture.";
    return false;
  }
  if (!mapping) {
    return false;
  }
  if (!OnSetContents(std::move(mapping), slice)) {
    return false;
  }
  coordinate_system_ = TextureCoordinateSystem::kUploadFromHost;
  is_opaque_ = is_opaque;
  return true;
}

const TextureDescriptor& TextureVK::GetTextureDescriptor() const {
  return desc_;
}

bool TextureVK::IsOpaque() const {
  return is_opaque_;
}

size_t TextureVK::GetMipCount() const {
  return GetTextureDescriptor().mip_count;
}

void TextureVK::SetCoordinateSystem(TextureCoordinateSystem coordinate_system) {
  coordinate_system_ = coordinate_system;
}

TextureCoordinateSystem TextureVK::GetCoordinateSystem() const {
  return coordinate_system_;
}

Scalar TextureVK::GetYCoordScale() const {
  return 1.0;
}

bool TextureVK::NeedsMipmapGeneration() const {
  return !mipmap_generated_ && desc_.mip_count > 1;
}

bool TextureVK::OnSetContents(const uint8_t* contents,
                              size_t length,
                              size_t slice) {
  if (!IsValid() || !contents) {
    return false;
  }

  const auto& desc = GetTextureDescriptor();

  // Out of bounds access.
  if (length != desc.GetByteSizeOfBaseMipLevel()) {
    LOG(ERROR) << "Illegal to set contents for invalid size.";
    return false;
  }

  auto context = context_.lock();
  if (!context) {
    LOG(ERROR) << "Context died before setting contents on texture.";
    return false;
  }

  auto staging_buffer =
      context->GetResourceAllocator()->CreateBufferWithCopy(contents, length);

  if (!staging_buffer) {
    LOG(ERROR) << "Could not create staging buffer.";
    return false;
  }

  auto cmd_buffer = context->CreateCommandBuffer();

  if (!cmd_buffer) {
    return false;
  }

  if (!cmd_buffer->Track(staging_buffer) || !cmd_buffer->Track(source_)) {
    return false;
  }

  const auto& vk_cmd_buffer = cmd_buffer->GetCommandBuffer();

  Barrier barrier;
  barrier.cmd_buffer = vk_cmd_buffer;
  barrier.new_layout = vk::ImageLayout::eTransferDstOptimal;
  barrier.src_access = {};
  barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
  barrier.dst_access = vk::AccessFlagBits::eTransferWrite;
  barrier.dst_stage = vk::PipelineStageFlagBits::eTransfer;

  if (!SetLayout(barrier)) {
    return false;
  }

  vk::BufferImageCopy copy;
  copy.bufferOffset = 0u;
  copy.bufferRowLength = 0u;    // 0u means tightly packed per spec.
  copy.bufferImageHeight = 0u;  // 0u means tightly packed per spec.
  copy.imageOffset.x = 0u;
  copy.imageOffset.y = 0u;
  copy.imageOffset.z = 0u;
  copy.imageExtent.width = desc.size.width;
  copy.imageExtent.height = desc.size.height;
  copy.imageExtent.depth = 1u;
  copy.imageSubresource.aspectMask =
      ToImageAspectFlags(GetTextureDescriptor().format);
  copy.imageSubresource.mipLevel = 0u;
  copy.imageSubresource.baseArrayLayer = slice;
  copy.imageSubresource.layerCount = 1u;

  vk_cmd_buffer.copyBufferToImage(staging_buffer->GetBuffer(),  // src buffer
                                  GetImage(),                   // dst image
                                  barrier.new_layout,  // dst image layout
                                  1u,                  // region count
                                  &copy                // regions
  );

  // Transition to shader-read.
  {
    Barrier barrier;
    barrier.cmd_buffer = vk_cmd_buffer;
    barrier.src_access = vk::AccessFlagBits::eColorAttachmentWrite |
                         vk::AccessFlagBits::eTransferWrite;
    barrier.src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                        vk::PipelineStageFlagBits::eTransfer;
    barrier.dst_access = vk::AccessFlagBits::eShaderRead;
    barrier.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

    barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

    if (!SetLayout(barrier)) {
      return false;
    }
  }

  return context->GetCommandQueue()->Submit({cmd_buffer}).ok();
}

bool TextureVK::OnSetContents(std::shared_ptr<const fml::Mapping> mapping,
                              size_t slice) {
  // Vulkan has no threading restrictions. So we can pass this data along to the
  // client rendering API immediately.
  return OnSetContents(mapping->GetMapping(), mapping->GetSize(), slice);
}

bool TextureVK::IsValid() const {
  return !!source_;
}

ISize TextureVK::GetSize() const {
  return GetTextureDescriptor().size;
}

vk::Image TextureVK::GetImage() const {
  return source_->GetImage();
}

vk::ImageView TextureVK::GetImageView() const {
  return source_->GetImageView();
}

std::shared_ptr<const TextureSource> TextureVK::GetTextureSource() const {
  return source_;
}

bool TextureVK::SetLayout(const Barrier& barrier) const {
  return source_ ? source_->SetLayout(barrier).ok() : false;
}

vk::ImageLayout TextureVK::SetLayoutWithoutEncoding(
    vk::ImageLayout layout) const {
  return source_ ? source_->SetLayoutWithoutEncoding(layout)
                 : vk::ImageLayout::eUndefined;
}

vk::ImageLayout TextureVK::GetLayout() const {
  return source_ ? source_->GetLayout() : vk::ImageLayout::eUndefined;
}

vk::ImageView TextureVK::GetRenderTargetView() const {
  return source_->GetRenderTargetView();
}

void TextureVK::SetCachedFrameData(const FramebufferAndRenderPass& data,
                                   SampleCount sample_count) {
  source_->SetCachedFrameData(data, sample_count);
}

const FramebufferAndRenderPass& TextureVK::GetCachedFrameData(
    SampleCount sample_count) const {
  return source_->GetCachedFrameData(sample_count);
}

void TextureVK::SetMipMapGenerated() {
  mipmap_generated_ = true;
}

bool TextureVK::IsSwapchainImage() const {
  return source_->IsSwapchainImage();
}

std::shared_ptr<Sampler> TextureVK::GetImmutableSamplerVariant(
    const Sampler& sampler) const {
  if (!source_) {
    return nullptr;
  }
  std::shared_ptr<YUVConversion> conversion = source_->GetYUVConversion();
  if (!conversion) {
    // Most textures don't need a sampler conversion and will go down this path.
    // Only needed for YUV sampling from external textures.
    return nullptr;
  }
  return sampler.CreateVariantForConversion(std::move(conversion));
}

}  // namespace ogre

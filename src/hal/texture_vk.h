// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string_view>

#include "core/formats.h"
#include "core/texture_descriptor.h"
#include "fml/mapping.h"
#include "geometry/size.h"
#include "hal/context_vk.h"
#include "hal/device_buffer_vk.h"
#include "hal/formats_vk.h"
#include "hal/sampler_vk.h"
#include "hal/texture_source_vk.h"
#include "hal/vk.h"

namespace ogre {

class TextureVK final {
 public:
  TextureVK(std::weak_ptr<Context> context,
            std::shared_ptr<TextureSource> source);

  ~TextureVK();

  /// @brief Label this resource for inspection in GPU debugging tools.
  ///
  /// This functionality may be disabled in release builds.
  void SetLabel(std::string_view label);

  /// @brief Label this resource for inspection in GPU debugging tools, with
  ///        label and trailing will be concatenated together.
  ///
  /// This functionality may be disabled in release builds.
  void SetLabel(std::string_view label, std::string_view trailing);

  bool IsValid() const;

  ISize GetSize() const;

  const TextureDescriptor& GetTextureDescriptor() const;

  bool IsOpaque() const;

  size_t GetMipCount() const;

  void SetCoordinateSystem(TextureCoordinateSystem coordinate_system);

  TextureCoordinateSystem GetCoordinateSystem() const;

  Scalar GetYCoordScale() const;

  bool NeedsMipmapGeneration() const;

  // Deprecated: use BlitPass::AddCopy instead.
  [[nodiscard]] bool SetContents(const uint8_t* contents,
                                 size_t length,
                                 size_t slice = 0,
                                 bool is_opaque = false);

  // Deprecated: use BlitPass::AddCopy instead.
  [[nodiscard]] bool SetContents(std::shared_ptr<const fml::Mapping> mapping,
                                 size_t slice = 0,
                                 bool is_opaque = false);

  vk::Image GetImage() const;

  vk::ImageView GetImageView() const;

  vk::ImageView GetRenderTargetView() const;

  bool SetLayout(const Barrier& barrier) const;

  vk::ImageLayout SetLayoutWithoutEncoding(vk::ImageLayout layout) const;

  vk::ImageLayout GetLayout() const;

  std::shared_ptr<const TextureSource> GetTextureSource() const;

  void SetMipMapGenerated();

  bool IsSwapchainImage() const;

  std::shared_ptr<Sampler> GetImmutableSamplerVariant(
      const Sampler& sampler) const;

  /// Store the last framebuffer and render pass object used with this texture.
  ///
  /// This method is only called if this texture is used as the resolve texture
  /// of a render pass. By construction, this framebuffer should be compatible
  /// with any future render passes.
  void SetCachedFrameData(const FramebufferAndRenderPass& data,
                          SampleCount sample_count);

  /// Retrieve the last framebuffer and render pass object used with this
  /// texture.
  ///
  /// An empty FramebufferAndRenderPass is returned if there is no cached data
  /// for a particular sample count.
  const FramebufferAndRenderPass& GetCachedFrameData(
      SampleCount sample_count) const;

 private:
  std::weak_ptr<Context> context_;
  const TextureDescriptor desc_;
  std::shared_ptr<TextureSource> source_;
  bool has_validation_layers_ = false;
  TextureCoordinateSystem coordinate_system_ =
      TextureCoordinateSystem::kRenderToTexture;
  bool is_opaque_ = false;
  bool mipmap_generated_ = false;

  bool OnSetContents(const uint8_t* contents, size_t length, size_t slice);

  bool OnSetContents(std::shared_ptr<const fml::Mapping> mapping, size_t slice);

  bool IsSliceValid(size_t slice) const;

  TextureVK(const TextureVK&) = delete;

  TextureVK& operator=(const TextureVK&) = delete;
};

}  // namespace ogre

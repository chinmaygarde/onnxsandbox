// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string_view>

#include "core/formats.h"
#include "core/texture_descriptor.h"
#include "fml/mapping.h"
#include "geometry/size.h"

namespace ogre {

class Texture {
 public:
  virtual ~Texture();

  /// @brief Label this resource for inspection in GPU debugging tools.
  ///
  /// This functionality may be disabled in release builds.
  virtual void SetLabel(std::string_view label) = 0;

  /// @brief Label this resource for inspection in GPU debugging tools, with
  ///        label and trailing will be concatenated together.
  ///
  /// This functionality may be disabled in release builds.
  virtual void SetLabel(std::string_view label, std::string_view trailing) = 0;

  // Deprecated: use BlitPass::AddCopy instead.
  [[nodiscard]] bool SetContents(const uint8_t* contents,
                                 size_t length,
                                 size_t slice = 0,
                                 bool is_opaque = false);

  // Deprecated: use BlitPass::AddCopy instead.
  [[nodiscard]] bool SetContents(std::shared_ptr<const fml::Mapping> mapping,
                                 size_t slice = 0,
                                 bool is_opaque = false);

  virtual bool IsValid() const = 0;

  virtual ISize GetSize() const = 0;

  bool IsOpaque() const;

  size_t GetMipCount() const;

  const TextureDescriptor& GetTextureDescriptor() const;

  /// Update the coordinate system used by the texture.
  ///
  /// The setting is used to conditionally invert the coordinates to
  /// account for the different origin of GLES textures.
  void SetCoordinateSystem(TextureCoordinateSystem coordinate_system);

  TextureCoordinateSystem GetCoordinateSystem() const;

  virtual Scalar GetYCoordScale() const;

  /// Returns true if mipmaps have never been generated.
  /// The contents of the mipmap may be out of date if the root texture has been
  /// modified and the mipmaps hasn't been regenerated.
  bool NeedsMipmapGeneration() const;

 protected:
  explicit Texture(TextureDescriptor desc);

  [[nodiscard]] virtual bool OnSetContents(const uint8_t* contents,
                                           size_t length,
                                           size_t slice) = 0;

  [[nodiscard]] virtual bool OnSetContents(
      std::shared_ptr<const fml::Mapping> mapping,
      size_t slice) = 0;

  bool mipmap_generated_ = false;

 private:
  TextureCoordinateSystem coordinate_system_ =
      TextureCoordinateSystem::kRenderToTexture;
  const TextureDescriptor desc_;
  bool is_opaque_ = false;

  bool IsSliceValid(size_t slice) const;

  Texture(const Texture&) = delete;

  Texture& operator=(const Texture&) = delete;
};

}  // namespace ogre

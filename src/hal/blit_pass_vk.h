// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/config.h"
#include "core/buffer_view.h"
#include "fml/macros.h"
#include "geometry/point.h"
#include "geometry/rect.h"
#include "hal/device_buffer_vk.h"
#include "hal/texture_vk.h"
#include "hal/workarounds_vk.h"

namespace ogre {

class CommandEncoder;
class CommandBuffer;

class BlitPass final {
 public:
  ~BlitPass();

  bool IsValid() const;

  void SetLabel(std::string_view label);

  bool ConvertTextureToShaderRead(const std::shared_ptr<TextureVK>& texture);

  bool ResizeTexture(const std::shared_ptr<TextureVK>& source,
                     const std::shared_ptr<TextureVK>& destination);

  bool AddCopy(std::shared_ptr<TextureVK> source,
               std::shared_ptr<TextureVK> destination,
               std::optional<IRect> source_region = std::nullopt,
               IPoint destination_origin = {},
               std::string_view label = "");

  bool AddCopy(std::shared_ptr<TextureVK> source,
               std::shared_ptr<DeviceBufferVK> destination,
               std::optional<IRect> source_region = std::nullopt,
               size_t destination_offset = 0,
               std::string_view label = "");

  bool AddCopy(BufferView source,
               std::shared_ptr<TextureVK> destination,
               std::optional<IRect> destination_region = std::nullopt,
               std::string_view label = "",
               uint32_t mip_level = 0,
               uint32_t slice = 0,
               bool convert_to_read = true);

  bool GenerateMipmap(std::shared_ptr<TextureVK> texture,
                      std::string_view label = "");

  bool EncodeCommands() const;

 private:
  friend class CommandBuffer;
  FML_FRIEND_TEST(BlitPassTest, MipmapGenerationTransitionsAllLevelsCorrectly);

  std::shared_ptr<CommandBuffer> command_buffer_;
  const Workarounds workarounds_;

  explicit BlitPass(std::shared_ptr<CommandBuffer> command_buffer,
                    const Workarounds& workarounds);

  void OnSetLabel(std::string_view label);

  bool OnCopyTextureToTextureCommand(std::shared_ptr<TextureVK> source,
                                     std::shared_ptr<TextureVK> destination,
                                     IRect source_region,
                                     IPoint destination_origin,
                                     std::string_view label);

  bool OnCopyTextureToBufferCommand(std::shared_ptr<TextureVK> source,
                                    std::shared_ptr<DeviceBufferVK> destination,
                                    IRect source_region,
                                    size_t destination_offset,
                                    std::string_view label);

  bool OnCopyBufferToTextureCommand(BufferView source,
                                    std::shared_ptr<TextureVK> destination,
                                    IRect destination_region,
                                    std::string_view label,
                                    uint32_t mip_level,
                                    uint32_t slice,
                                    bool convert_to_read);

  bool OnGenerateMipmapCommand(std::shared_ptr<TextureVK> texture,
                               std::string_view label);

  BlitPass(const BlitPass&) = delete;

  BlitPass& operator=(const BlitPass&) = delete;
};

}  // namespace ogre

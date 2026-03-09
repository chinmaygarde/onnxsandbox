// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include "geometry/rect.h"
#include "hal/device_buffer_vk.h"
#include "hal/texture_vk.h"

namespace ogre {

struct BlitCommand {
  std::string label;
};

struct BlitCopyTextureToTextureCommand : public BlitCommand {
  std::shared_ptr<TextureVK> source;
  std::shared_ptr<TextureVK> destination;
  IRect source_region;
  IPoint destination_origin;
};

struct BlitResizeTextureCommand : public BlitCommand {
  std::shared_ptr<TextureVK> source;
  std::shared_ptr<TextureVK> destination;
};

struct BlitCopyTextureToBufferCommand : public BlitCommand {
  std::shared_ptr<TextureVK> source;
  std::shared_ptr<DeviceBufferVK> destination;
  IRect source_region;
  size_t destination_offset;
};

struct BlitCopyBufferToTextureCommand : public BlitCommand {
  BufferView source;
  std::shared_ptr<TextureVK> destination;
  IRect destination_region;
  uint32_t mip_level = 0;
  uint32_t slice = 0;
};

struct BlitGenerateMipmapCommand : public BlitCommand {
  std::shared_ptr<TextureVK> texture;
};

}  // namespace ogre

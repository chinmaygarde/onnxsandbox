// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "fml/status.h"
#include "hal/context_vk.h"
#include "hal/texture_vk.h"

namespace ogre {

class CommandBuffer;

std::shared_ptr<TextureVK> CreateTexture(
    const TextureDescriptor& texture_descriptor,
    const std::vector<uint8_t>& data,
    const std::shared_ptr<ogre::Context>& context,
    std::string_view debug_label);

/// Adds a blit command to the render pass.
[[nodiscard]] fml::Status AddMipmapGeneration(
    const std::shared_ptr<CommandBuffer>& command_buffer,
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<TextureVK>& texture);

}  // namespace ogre

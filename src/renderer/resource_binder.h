// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "core/buffer_view.h"
#include "core/formats.h"
#include "core/raw_ptr.h"
#include "core/shader_types.h"
#include "renderer/sampler_vk.h"
#include "renderer/texture_vk.h"

namespace ogre {

//------------------------------------------------------------------------------
/// @brief      An interface for binding resources. This is implemented by
///             |Command| and |ComputeCommand| to make GPU resources available
///             to a given command's pipeline.
///
struct ResourceBinder {
  virtual ~ResourceBinder() = default;

  virtual bool BindResource(ShaderStage stage,
                            DescriptorType type,
                            const ShaderUniformSlot& slot,
                            const ShaderMetadata* metadata,
                            BufferView view) = 0;

  virtual bool BindResource(ShaderStage stage,
                            DescriptorType type,
                            const SampledImageSlot& slot,
                            const ShaderMetadata* metadata,
                            std::shared_ptr<const TextureVK> texture,
                            raw_ptr<const Sampler>) = 0;
};

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/shader_types.h"
#include "renderer/vk.h"

namespace ogre {

vk::Format ToVertexDescriptorFormat(const ShaderStageIOSlot& input);

}  // namespace ogre

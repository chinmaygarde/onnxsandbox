// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VERTEX_DESCRIPTOR_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VERTEX_DESCRIPTOR_VK_H_

#include "core/shader_types.h"
#include "renderer/backend/vulkan/vk.h"

namespace ogre {

vk::Format ToVertexDescriptorFormat(const ShaderStageIOSlot& input);

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VERTEX_DESCRIPTOR_VK_H_

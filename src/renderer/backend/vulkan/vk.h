// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VK_H_

#include "fml/build_config.h"
#include "fml/logging.h"

#define VK_NO_PROTOTYPES

#if !defined(NDEBUG)
#define VULKAN_HPP_ASSERT FML_CHECK
#else
#define VULKAN_HPP_ASSERT(ignored) \
  {                                \
  }
#endif

#define VULKAN_HPP_NAMESPACE ogre::vk
#define VULKAN_HPP_ASSERT_ON_RESULT(ignored) \
  {                                          \
    [[maybe_unused]] auto res = (ignored);   \
  }
#define VULKAN_HPP_NO_EXCEPTIONS

// The spaceship operator behaves differently on 32-bit platforms.
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR

#include <vulkan/vulkan.hpp>  // IWYU pragma: keep.

// The Vulkan headers may bring in X11 headers which define some macros that
// conflict with other code.  Undefine these macros after including Vulkan.
#undef Bool
#undef None
#undef Status
#undef Success

static_assert(VK_HEADER_VERSION >= 215, "Vulkan headers must not be too old.");

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_VK_H_

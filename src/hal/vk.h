// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <absl/log/check.h>
#include "fml/build_config.h"

#define VK_NO_PROTOTYPES

#if !defined(NDEBUG)
#define VULKAN_HPP_ASSERT CHECK
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

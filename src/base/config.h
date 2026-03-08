// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdlib>

#include <absl/log/check.h>
#include "fml/logging.h"

#if defined(__GNUC__) || defined(__clang__)
#define OGRE_COMPILER_CLANG 1
#else  // defined(__GNUC__) || defined(__clang__)
#define OGRE_COMPILER_CLANG 0
#endif  // defined(__GNUC__) || defined(__clang__)

#if OGRE_COMPILER_CLANG
#define OGRE_PRINTF_FORMAT(format_number, args_number) \
  __attribute__((format(printf, format_number, args_number)))
#else  // OGRE_COMPILER_CLANG
#define OGRE_PRINTF_FORMAT(format_number, args_number)
#endif  // OGRE_COMPILER_CLANG

#define OGRE_UNIMPLEMENTED \
  ogre::OgreUnimplemented(__FUNCTION__, __FILE__, __LINE__);

namespace ogre {

[[noreturn]] inline void OgreUnimplemented(const char* method,
                                           const char* file,
                                           int line) {
  CHECK(false) << "Unimplemented: " << method << " in " << file << ":" << line;
  std::abort();
}

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <sys/types.h>

#include "fml/macros.h"

namespace fml {

bool IsPlatformVersionAtLeast(size_t major, size_t minor = 0, size_t patch = 0);

}  // namespace fml

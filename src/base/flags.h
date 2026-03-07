// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_BASE_FLAGS_H_
#define FLUTTER_OGRE_BASE_FLAGS_H_

namespace ogre {
struct Flags {
  /// When turned on DrawLine will use the experimental antialiased path.
  bool antialiased_lines = false;
};
}  // namespace ogre

#endif  // FLUTTER_OGRE_BASE_FLAGS_H_

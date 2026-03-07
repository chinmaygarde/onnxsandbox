// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_GEOMETRY_MATRIX_DECOMPOSITION_H_
#define FLUTTER_OGRE_GEOMETRY_MATRIX_DECOMPOSITION_H_

#include "geometry/quaternion.h"
#include "geometry/scalar.h"
#include "geometry/shear.h"
#include "geometry/vector.h"

namespace ogre {

struct MatrixDecomposition {
  Vector3 translation;
  Vector3 scale;
  Shear shear;
  Vector4 perspective;
  Quaternion rotation;

  enum class Component {
    kTranslation = 1 << 0,
    kScale = 1 << 1,
    kShear = 1 << 2,
    kPerspective = 1 << 3,
    kRotation = 1 << 4,
  };

  uint64_t GetComponentsMask() const;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_GEOMETRY_MATRIX_DECOMPOSITION_H_

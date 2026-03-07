// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_GEOMETRY_GRADIENT_H_
#define FLUTTER_OGRE_GEOMETRY_GRADIENT_H_

#include <cstdint>
#include <vector>

#include "geometry/color.h"

namespace ogre {

// If texture_size is 0 then the gradient is invalid.
struct GradientData {
  std::vector<uint8_t> color_bytes;
  uint32_t texture_size;
};

/**
 * @brief Populate a vector with the interpolated color bytes for the linear
 * gradient described by colors and stops.
 *
 * @param colors
 * @param stops
 * @return GradientData
 */
GradientData CreateGradientBuffer(const std::vector<Color>& colors,
                                  const std::vector<Scalar>& stops);

}  // namespace ogre

#endif  // FLUTTER_OGRE_GEOMETRY_GRADIENT_H_

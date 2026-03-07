// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_GEOMETRY_STROKE_PARAMETERS_H_
#define FLUTTER_OGRE_GEOMETRY_STROKE_PARAMETERS_H_

#include "geometry/scalar.h"

namespace ogre {

/// @brief An enum that describes ways to decorate the end of a path contour.
enum class Cap {
  kButt,
  kRound,
  kSquare,
};

/// @brief An enum that describes ways to join two segments of a path.
enum class Join {
  kMiter,
  kRound,
  kBevel,
};

/// @brief A structure to store all of the parameters related to stroking
///        a path or basic geometry object.
struct StrokeParameters {
  Scalar width = 0.0f;
  Cap cap = Cap::kButt;
  Join join = Join::kMiter;
  Scalar miter_limit = 4.0f;

  constexpr bool operator==(const StrokeParameters& parameters) const = default;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_GEOMETRY_STROKE_PARAMETERS_H_

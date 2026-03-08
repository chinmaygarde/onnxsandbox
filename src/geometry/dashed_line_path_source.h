// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "geometry/path_source.h"
#include "geometry/point.h"
#include "geometry/scalar.h"

namespace ogre {

/// @brief A PathSource that generates the various segments of a dashed line.
class DashedLinePathSource : public PathSource {
 public:
  DashedLinePathSource(Point p0, Point p1, Scalar on_length, Scalar off_length);

  ~DashedLinePathSource();

  // |PathSource|
  FillType GetFillType() const override;

  // |PathSource|
  Rect GetBounds() const override;

  // |PathSource|
  bool IsConvex() const override;

  // |PathSource|
  void Dispatch(PathReceiver& receiver) const override;

 private:
  const Point p0_;
  const Point p1_;
  const Scalar on_length_;
  const Scalar off_length_;
};

}  // namespace ogre

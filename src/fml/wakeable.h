// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "fml/time/time_point.h"

namespace fml {

/// Interface over the ability to \p WakeUp a \p fml::MessageLoopImpl.
/// \see fml::MessageLoopTaskQueues
class Wakeable {
 public:
  virtual ~Wakeable() {}

  virtual void WakeUp(fml::TimePoint time_point) = 0;
};

}  // namespace fml

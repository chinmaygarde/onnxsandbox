// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "fml/macros.h"

namespace fml {

// Pushes an autorelease pool when constructed and pops it when destructed.
class ScopedNSAutoreleasePool {
 public:
  ScopedNSAutoreleasePool();
  ~ScopedNSAutoreleasePool();

 private:
  void* autorelease_pool_;

  FML_DISALLOW_COPY_AND_ASSIGN(ScopedNSAutoreleasePool);
};

}  // namespace fml

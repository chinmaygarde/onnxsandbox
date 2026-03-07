// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_BASE_BACKEND_CAST_H_
#define FLUTTER_OGRE_BASE_BACKEND_CAST_H_

namespace ogre {

template <class Sub, class Base>
class BackendCast {
 public:
  static Sub& Cast(Base& base) { return reinterpret_cast<Sub&>(base); }

  static const Sub& Cast(const Base& base) {
    return reinterpret_cast<const Sub&>(base);
  }

  static Sub* Cast(Base* base) { return reinterpret_cast<Sub*>(base); }

  static const Sub* Cast(const Base* base) {
    return reinterpret_cast<const Sub*>(base);
  }
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_BASE_BACKEND_CAST_H_

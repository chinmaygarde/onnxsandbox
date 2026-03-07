// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_BASE_STRINGS_H_
#define FLUTTER_OGRE_BASE_STRINGS_H_

#include <string>

namespace ogre {

bool HasPrefix(const std::string& string, const std::string& prefix);

bool HasSuffix(const std::string& string, const std::string& suffix);

std::string StripPrefix(const std::string& string, const std::string& to_strip);

}  // namespace ogre

#endif  // FLUTTER_OGRE_BASE_STRINGS_H_

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_CORE_DEVICE_BUFFER_DESCRIPTOR_H_
#define FLUTTER_OGRE_CORE_DEVICE_BUFFER_DESCRIPTOR_H_

#include <cstddef>

#include "core/formats.h"

namespace ogre {

struct DeviceBufferDescriptor {
  StorageMode storage_mode = StorageMode::kDeviceTransient;
  size_t size = 0u;
  // Perhaps we could combine this with storage mode and create appropriate
  // host-write and host-read flags.
  bool readback = false;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_CORE_DEVICE_BUFFER_DESCRIPTOR_H_

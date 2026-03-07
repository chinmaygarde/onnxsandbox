// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_CORE_BUFFER_VIEW_H_
#define FLUTTER_OGRE_CORE_BUFFER_VIEW_H_

#include <memory>
#include "core/range.h"

namespace ogre {

class DeviceBuffer;

/// A specific range in a DeviceBuffer.
///
/// BufferView can maintain ownership over the DeviceBuffer or not depending on
/// if it is created with a std::shared_ptr or a raw pointer.
struct BufferView {
 public:
  BufferView();

  BufferView(DeviceBuffer* buffer, Range range);

  BufferView(std::shared_ptr<const DeviceBuffer> buffer, Range range);

  Range GetRange() const { return range_; }

  const DeviceBuffer* GetBuffer() const;

  std::shared_ptr<const DeviceBuffer> TakeBuffer();

  explicit operator bool() const;

 private:
  std::shared_ptr<const DeviceBuffer> buffer_;
  /// This is a non-owned DeviceBuffer. Steps should be taken to make sure this
  /// lives for the duration of the BufferView's life. Usually this is done
  /// automatically by the graphics API or in the case of Vulkan the HostBuffer
  /// or TrackedObjectsVK keeps it alive.
  const DeviceBuffer* raw_buffer_;
  Range range_;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_CORE_BUFFER_VIEW_H_

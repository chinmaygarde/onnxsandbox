// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "core/range.h"

namespace ogre {

class DeviceBufferVK;

/// A specific range in a DeviceBufferVK.
///
/// BufferView can maintain ownership over the DeviceBufferVK or not depending
/// on if it is created with a std::shared_ptr or a raw pointer.
struct BufferView {
 public:
  BufferView();

  BufferView(DeviceBufferVK* buffer, Range range);

  BufferView(std::shared_ptr<const DeviceBufferVK> buffer, Range range);

  Range GetRange() const { return range_; }

  const DeviceBufferVK* GetBuffer() const;

  std::shared_ptr<const DeviceBufferVK> TakeBuffer();

  explicit operator bool() const;

 private:
  std::shared_ptr<const DeviceBufferVK> buffer_;
  /// This is a non-owned DeviceBufferVK. Steps should be taken to make sure
  /// this lives for the duration of the BufferView's life. Usually this is done
  /// automatically by the graphics API or in the case of Vulkan the HostBuffer
  /// or TrackedObjects keeps it alive.
  const DeviceBufferVK* raw_buffer_;
  Range range_;
};

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include "core/buffer_view.h"

namespace ogre {
namespace testing {

TEST(BufferViewTest, Empty) {
  BufferView buffer_view;
  EXPECT_FALSE(buffer_view);
}

TEST(BufferViewTest, TakeRaw) {
  DeviceBufferVK* buffer = reinterpret_cast<DeviceBufferVK*>(0xcafebabe);
  BufferView buffer_view(buffer, {0, 123});
  EXPECT_TRUE(buffer_view);
  std::shared_ptr<const DeviceBufferVK> taken = buffer_view.TakeBuffer();
  EXPECT_FALSE(taken);
  EXPECT_EQ(buffer_view.GetBuffer(), buffer);
}

}  // namespace testing
}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "renderer/backend/vulkan/device_holder_vk.h"

namespace ogre {

class IdleWaiter {
 public:
  explicit IdleWaiter(std::weak_ptr<DeviceHolder> device_holder)
      : device_holder_(std::move(device_holder)) {}

  void WaitIdle() const {
    std::shared_ptr<DeviceHolder> strong_device_holder = device_holder_.lock();
    if (strong_device_holder && strong_device_holder->GetDevice()) {
      [[maybe_unused]] auto result =
          strong_device_holder->GetDevice().waitIdle();
    }
  }

 private:
  std::weak_ptr<DeviceHolder> device_holder_;
};

}  // namespace ogre

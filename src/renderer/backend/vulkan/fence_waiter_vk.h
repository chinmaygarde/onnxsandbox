// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#include "fml/closure.h"
#include "renderer/backend/vulkan/device_holder_vk.h"

namespace ogre {

class Context;
class WaitSetEntry;

using WaitSet = std::vector<std::shared_ptr<WaitSetEntry>>;

class FenceWaiter {
 public:
  ~FenceWaiter();

  bool IsValid() const;

  void Terminate();

  bool AddFence(vk::UniqueFence fence, const fml::closure& callback);

 private:
  friend class Context;

  std::weak_ptr<DeviceHolder> device_holder_;
  std::unique_ptr<std::thread> waiter_thread_;
  std::mutex wait_set_mutex_;
  std::condition_variable wait_set_cv_;
  WaitSet wait_set_;
  bool terminate_ = false;

  explicit FenceWaiter(std::weak_ptr<DeviceHolder> device_holder);

  void Main();

  bool Wait();
  void WaitUntilEmpty();

  FenceWaiter(const FenceWaiter&) = delete;

  FenceWaiter& operator=(const FenceWaiter&) = delete;
};

}  // namespace ogre

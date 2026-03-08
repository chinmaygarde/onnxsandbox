// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "fml/status.h"
#include "renderer/backend/vulkan/command_buffer_vk.h"

namespace ogre {

class Context;

class CommandQueue {
 public:
  using CompletionCallback = std::function<void(CommandBuffer::Status)>;

  explicit CommandQueue(const std::weak_ptr<Context>& context);

  ~CommandQueue();

  fml::Status Submit(const std::vector<std::shared_ptr<CommandBuffer>>& buffers,
                     const CompletionCallback& completion_callback = {},
                     bool block_on_schedule = false);

 private:
  std::weak_ptr<Context> context_;

  CommandQueue(const CommandQueue&) = delete;

  CommandQueue& operator=(const CommandQueue&) = delete;
};

}  // namespace ogre

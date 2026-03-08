// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/thread.h"
#include "renderer/backend/vulkan/vk.h"

namespace ogre {

struct QueueIndex {
  size_t family = 0;
  size_t index = 0;

  constexpr bool operator==(const QueueIndex& other) const {
    return family == other.family && index == other.index;
  }
};

//------------------------------------------------------------------------------
/// @brief      A thread safe object that can be used to access device queues.
///             If multiple objects are created with the same underlying queue,
///             then the external synchronization guarantees of Vulkan queues
///             cannot be met. So care must be taken the same device queue
///             doesn't form the basis of multiple `Queue`s.
///
class Queue {
 public:
  Queue(QueueIndex index, vk::Queue queue);

  ~Queue();

  const QueueIndex& GetIndex() const;

  vk::Result Submit(const vk::SubmitInfo& submit_info,
                    const vk::Fence& fence) const;

  vk::Result Submit(const vk::Fence& fence) const;

  vk::Result Present(const vk::PresentInfoKHR& present_info);

  void InsertDebugMarker(std::string_view label) const;

 private:
  mutable Mutex queue_mutex_;

  const QueueIndex index_;
  const vk::Queue queue_ IPLR_GUARDED_BY(queue_mutex_);

  Queue(const Queue&) = delete;

  Queue& operator=(const Queue&) = delete;
};

//------------------------------------------------------------------------------
/// @brief      The collection of queues used by the context. The queues may all
///             be the same.
///
struct QueuesVK {
  std::shared_ptr<Queue> graphics_queue;
  std::shared_ptr<Queue> compute_queue;
  std::shared_ptr<Queue> transfer_queue;

  QueuesVK();

  QueuesVK(std::shared_ptr<Queue> graphics_queue,
           std::shared_ptr<Queue> compute_queue,
           std::shared_ptr<Queue> transfer_queue);

  static QueuesVK FromEmbedderQueue(vk::Queue queue,
                                    uint32_t queue_family_index);

  static QueuesVK FromQueueIndices(const vk::Device& device,
                                   QueueIndex graphics,
                                   QueueIndex compute,
                                   QueueIndex transfer);

  bool IsValid() const;
};

}  // namespace ogre

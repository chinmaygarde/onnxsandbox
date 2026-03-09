// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/queue_vk.h"

#include <utility>

#include "hal/context_vk.h"

namespace ogre {

Queue::Queue(QueueIndex index, vk::Queue queue)
    : index_(index), queue_(queue) {}

Queue::~Queue() = default;

const QueueIndex& Queue::GetIndex() const {
  return index_;
}

vk::Result Queue::Submit(const vk::SubmitInfo& submit_info,
                         const vk::Fence& fence) const {
  Lock lock(queue_mutex_);
  return queue_.submit(submit_info, fence);
}

vk::Result Queue::Submit(const vk::Fence& fence) const {
  Lock lock(queue_mutex_);
  return queue_.submit({}, fence);
}

vk::Result Queue::Present(const vk::PresentInfoKHR& present_info) {
  Lock lock(queue_mutex_);
  return queue_.presentKHR(present_info);
}

void Queue::InsertDebugMarker(std::string_view label) const {
  if (!HasValidationLayers()) {
    return;
  }
  vk::DebugUtilsLabelEXT label_info;
  label_info.pLabelName = label.data();
  Lock lock(queue_mutex_);
  queue_.insertDebugUtilsLabelEXT(label_info);
}

QueuesVK::QueuesVK() = default;

QueuesVK::QueuesVK(std::shared_ptr<Queue> graphics_queue,
                   std::shared_ptr<Queue> compute_queue,
                   std::shared_ptr<Queue> transfer_queue)
    : graphics_queue(std::move(graphics_queue)),
      compute_queue(std::move(compute_queue)),
      transfer_queue(std::move(transfer_queue)) {}

// static
QueuesVK QueuesVK::FromEmbedderQueue(vk::Queue queue,
                                     uint32_t queue_family_index) {
  auto graphics_queue = std::make_shared<Queue>(
      QueueIndex{.family = queue_family_index, .index = 0}, queue);

  return QueuesVK(graphics_queue, graphics_queue, graphics_queue);
}

// static
QueuesVK QueuesVK::FromQueueIndices(const vk::Device& device,
                                    QueueIndex graphics,
                                    QueueIndex compute,
                                    QueueIndex transfer) {
  auto vk_graphics = device.getQueue(graphics.family, graphics.index);
  auto vk_compute = device.getQueue(compute.family, compute.index);
  auto vk_transfer = device.getQueue(transfer.family, transfer.index);

  // Always set up the graphics queue.
  auto graphics_queue = std::make_shared<Queue>(graphics, vk_graphics);
  Context::SetDebugName(device, vk_graphics, "ImpellerGraphicsQ");

  // Setup the compute queue if its different from the graphics queue.
  std::shared_ptr<Queue> compute_queue;
  if (compute == graphics) {
    compute_queue = graphics_queue;
  } else {
    compute_queue = std::make_shared<Queue>(compute, vk_compute);
    Context::SetDebugName(device, vk_compute, "ImpellerComputeQ");
  }

  // Setup the transfer queue if its different from the graphics or compute
  // queues.
  std::shared_ptr<Queue> transfer_queue;
  if (transfer == graphics) {
    transfer_queue = graphics_queue;
  } else if (transfer == compute) {
    transfer_queue = compute_queue;
  } else {
    transfer_queue = std::make_shared<Queue>(transfer, vk_transfer);
    Context::SetDebugName(device, vk_transfer, "ImpellerTransferQ");
  }

  return QueuesVK(std::move(graphics_queue), std::move(compute_queue),
                  std::move(transfer_queue));
}

bool QueuesVK::IsValid() const {
  return graphics_queue && compute_queue && transfer_queue;
}

}  // namespace ogre

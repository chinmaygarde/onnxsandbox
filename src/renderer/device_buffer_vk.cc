// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device_buffer_vk.h"

#include "core/formats.h"
#include "renderer/context_vk.h"
#include "renderer/vma.h"

namespace ogre {

DeviceBufferVK::DeviceBufferVK(DeviceBufferDescriptor desc,
                               std::weak_ptr<Context> context,
                               UniqueBufferVMA buffer,
                               VmaAllocationInfo info,
                               bool is_host_coherent)
    : desc_(desc),
      context_(std::move(context)),
      resource_((*context_.lock().get()).GetResourceManager(),
                BufferResource{
                    std::move(buffer),  //
                    info                //
                }),
      is_host_coherent_(is_host_coherent) {}

DeviceBufferVK::~DeviceBufferVK() = default;

uint8_t* DeviceBufferVK::OnGetContents() const {
  return static_cast<uint8_t*>(resource_->info.pMappedData);
}

bool DeviceBufferVK::CopyHostBuffer(const uint8_t* source,
                                    Range source_range,
                                    size_t offset) {
  if (source_range.length == 0u) {
    // Nothing to copy. Bail.
    return true;
  }

  if (source == nullptr) {
    // Attempted to copy data from a null buffer.
    return false;
  }

  if (desc_.storage_mode != StorageMode::kHostVisible) {
    // One of the storage modes where a transfer queue must be used.
    return false;
  }

  if (offset + source_range.length > desc_.size) {
    // Out of bounds of this buffer.
    return false;
  }

  uint8_t* dest = OnGetContents();

  if (!dest) {
    return false;
  }

  if (source) {
    ::memmove(dest + offset, source + source_range.offset, source_range.length);
  }
  ::vmaFlushAllocation(resource_->buffer.get().allocator,
                       resource_->buffer.get().allocation, offset,
                       source_range.length);

  return true;
}

bool DeviceBufferVK::SetLabel(std::string_view label) {
#ifdef OGRE_DEBUG
  auto context = context_.lock();
  if (!context || !resource_->buffer.is_valid()) {
    // The context could have died at this point.
    return false;
  }

  ::vmaSetAllocationName(resource_->buffer.get().allocator,   //
                         resource_->buffer.get().allocation,  //
                         label.data()                         //
  );

  return (*context).SetDebugName(resource_->buffer.get().buffer, label);
#else
  return true;
#endif  // OGRE_DEBUG
}

void DeviceBufferVK::Flush(std::optional<Range> range) const {
  if (is_host_coherent_) {
    return;
  }
  auto flush_range = range.value_or(Range{0, GetDeviceBufferDescriptor().size});
  ::vmaFlushAllocation(resource_->buffer.get().allocator,
                       resource_->buffer.get().allocation, flush_range.offset,
                       flush_range.length);
}

// Visible for testing.
bool DeviceBufferVK::IsHostCoherent() const {
  return is_host_coherent_;
}

void DeviceBufferVK::Invalidate(std::optional<Range> range) const {
  auto flush_range = range.value_or(Range{0, GetDeviceBufferDescriptor().size});
  ::vmaInvalidateAllocation(resource_->buffer.get().allocator,
                            resource_->buffer.get().allocation,
                            flush_range.offset, flush_range.length);
}

bool DeviceBufferVK::SetLabel(std::string_view label, Range range) {
  // We do not have the ability to name ranges. Just name the whole thing.
  return SetLabel(label);
}

vk::Buffer DeviceBufferVK::GetBuffer() const {
  return resource_->buffer.get().buffer;
}

// static
BufferView DeviceBufferVK::AsBufferView(
    std::shared_ptr<DeviceBufferVK> buffer) {
  Range range = {0u, buffer->desc_.size};
  return BufferView(std::move(buffer), range);
}

const DeviceBufferDescriptor& DeviceBufferVK::GetDeviceBufferDescriptor()
    const {
  return desc_;
}

}  // namespace ogre

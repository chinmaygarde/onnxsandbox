// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <optional>

#include "core/buffer_view.h"
#include "core/device_buffer_descriptor.h"
#include "core/range.h"
#include "renderer/resource_manager_vk.h"
#include "renderer/vma.h"

namespace ogre {

class Context;

class DeviceBufferVK final {
 public:
  DeviceBufferVK(DeviceBufferDescriptor desc,
                 std::weak_ptr<Context> context,
                 UniqueBufferVMA buffer,
                 VmaAllocationInfo info,
                 bool is_host_coherent);

  ~DeviceBufferVK();

  vk::Buffer GetBuffer() const;

  // Visible for testing.
  bool IsHostCoherent() const;

  [[nodiscard]] bool CopyHostBuffer(const uint8_t* source,
                                    Range source_range,
                                    size_t offset = 0u);

  bool SetLabel(std::string_view label);

  bool SetLabel(std::string_view label, Range range);

  /// @brief Create a buffer view of this entire buffer.
  static BufferView AsBufferView(std::shared_ptr<DeviceBufferVK> buffer);

  const DeviceBufferDescriptor& GetDeviceBufferDescriptor() const;

  uint8_t* OnGetContents() const;

  /// Make any pending writes visible to the GPU.
  void Flush(std::optional<Range> range = std::nullopt) const;

  void Invalidate(std::optional<Range> range = std::nullopt) const;

 private:
  friend class Allocator;

  struct BufferResource {
    UniqueBufferVMA buffer;
    VmaAllocationInfo info = {};

    BufferResource() = default;

    BufferResource(UniqueBufferVMA p_buffer, VmaAllocationInfo p_info)
        : buffer(std::move(p_buffer)), info(p_info) {}

    BufferResource(BufferResource&& o) {
      std::swap(o.buffer, buffer);
      std::swap(o.info, info);
    }

    BufferResource(const BufferResource&) = delete;

    BufferResource& operator=(const BufferResource&) = delete;
  };

  const DeviceBufferDescriptor desc_;
  std::weak_ptr<Context> context_;
  UniqueResourceVKT<BufferResource> resource_;
  bool is_host_coherent_ = false;

  DeviceBufferVK(const DeviceBufferVK&) = delete;

  DeviceBufferVK& operator=(const DeviceBufferVK&) = delete;
};

}  // namespace ogre

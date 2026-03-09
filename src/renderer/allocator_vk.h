// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/allocation_size.h"
#include "core/device_buffer_descriptor.h"
#include "core/texture_descriptor.h"
#include "fml/mapping.h"
#include "renderer/context_vk.h"
#include "renderer/device_buffer_vk.h"
#include "renderer/device_holder_vk.h"
#include "renderer/texture_vk.h"
#include "renderer/vk.h"

#include <cstdint>
#include <memory>

namespace ogre {

class Allocator final {
 public:
  // |Allocator|
  ~Allocator();

  bool IsValid() const;

  std::shared_ptr<DeviceBufferVK> CreateBuffer(
      const DeviceBufferDescriptor& desc);

  std::shared_ptr<TextureVK> CreateTexture(const TextureDescriptor& desc,
                                           bool threadsafe = false);

  std::shared_ptr<DeviceBufferVK> CreateBufferWithCopy(const uint8_t* buffer,
                                                       size_t length);

  std::shared_ptr<DeviceBufferVK> CreateBufferWithCopy(
      const fml::Mapping& mapping);

  uint16_t MinimumBytesPerRow(PixelFormat format) const;

  ISize GetMaxTextureSizeSupported() const;

  void DebugTraceMemoryStatistics() const;

  Bytes DebugGetHeapUsage() const;

  /// @brief Select a matching memory type for the given
  ///        [memory_type_bits_requirement], or -1 if none is found.
  ///
  ///        This only returns memory types with deviceLocal allocations.
  static int32_t FindMemoryTypeIndex(
      uint32_t memory_type_bits_requirement,
      vk::PhysicalDeviceMemoryProperties& memory_properties);

  // Visible for testing.
  static vk::ImageUsageFlags ToVKImageUsageFlags(
      PixelFormat format,
      TextureUsageMask usage,
      StorageMode mode,
      bool supports_memoryless_textures);

 private:
  friend class Context;

  UniqueAllocatorVMA allocator_;
  UniquePoolVMA staging_buffer_pool_;
  std::weak_ptr<Context> context_;
  std::weak_ptr<DeviceHolder> device_holder_;
  ISize max_texture_size_;
  bool is_valid_ = false;
  bool supports_memoryless_textures_ = false;
  // TODO(jonahwilliams): figure out why CI can't create these buffer pools.
  bool created_buffer_pool_ = true;
  vk::PhysicalDeviceMemoryProperties memory_properties_;

  Allocator(std::weak_ptr<Context> context,
            uint32_t vulkan_api_version,
            const vk::PhysicalDevice& physical_device,
            const std::shared_ptr<DeviceHolder>& device_holder,
            const vk::Instance& instance,
            const Capabilities& capabilities);

  std::shared_ptr<DeviceBufferVK> OnCreateBuffer(
      const DeviceBufferDescriptor& desc);

  std::shared_ptr<TextureVK> OnCreateTexture(const TextureDescriptor& desc,
                                             bool threadsafe);

  Allocator(const Allocator&) = delete;

  Allocator& operator=(const Allocator&) = delete;
};

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SAMPLER_LIBRARY_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SAMPLER_LIBRARY_VK_H_

#include "core/raw_ptr.h"
#include "core/sampler_descriptor.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/sampler_vk.h"
#include "renderer/backend/vulkan/workarounds_vk.h"

namespace ogre {

class SamplerLibrary final {
 public:
  ~SamplerLibrary();

  explicit SamplerLibrary(const std::weak_ptr<DeviceHolder>& device_holder);

  void ApplyWorkarounds(const Workarounds& workarounds);

 private:
  friend class Context;

  std::weak_ptr<DeviceHolder> device_holder_;
  std::vector<std::pair<uint64_t, std::shared_ptr<const Sampler>>> samplers_;
  bool mips_disabled_workaround_ = false;

  raw_ptr<const Sampler> GetSampler(const SamplerDescriptor& descriptor);

  SamplerLibrary(const SamplerLibrary&) = delete;

  SamplerLibrary& operator=(const SamplerLibrary&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SAMPLER_LIBRARY_VK_H_

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/sampler_library_vk.h"

#include "core/formats.h"
#include "renderer/sampler_vk.h"

namespace ogre {

SamplerLibrary::SamplerLibrary(const std::weak_ptr<DeviceHolder>& device_holder)
    : device_holder_(device_holder) {}

SamplerLibrary::~SamplerLibrary() = default;

void SamplerLibrary::ApplyWorkarounds(const Workarounds& workarounds) {
  mips_disabled_workaround_ = workarounds.broken_mipmap_generation;
}

raw_ptr<const Sampler> SamplerLibrary::GetSampler(
    const SamplerDescriptor& desc) {
  SamplerDescriptor desc_copy = desc;
  if (mips_disabled_workaround_) {
    desc_copy.mip_filter = MipFilter::kBase;
  }

  uint64_t p_key = SamplerDescriptor::ToKey(desc_copy);
  for (const auto& [key, value] : samplers_) {
    if (key == p_key) {
      return raw_ptr(value);
    }
  }
  auto device_holder = device_holder_.lock();
  if (!device_holder || !device_holder->GetDevice()) {
    return raw_ptr<const Sampler>(nullptr);
  }
  samplers_.push_back(std::make_pair(
      p_key, std::make_shared<Sampler>(device_holder->GetDevice(), desc_copy)));
  return raw_ptr(samplers_.back().second);
}

}  // namespace ogre

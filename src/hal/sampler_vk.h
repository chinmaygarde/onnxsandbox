// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/sampler_descriptor.h"
#include "hal/shared_object_vk.h"
#include "hal/vk.h"

namespace ogre {

class SamplerLibrary;
class YUVConversion;

class Sampler final {
 public:
  Sampler(const vk::Device& device,
          const SamplerDescriptor&,
          std::shared_ptr<YUVConversion> yuv_conversion = {});

  ~Sampler();

  const SamplerDescriptor& GetDescriptor() const;

  vk::Sampler GetSampler() const;

  std::shared_ptr<Sampler> CreateVariantForConversion(
      std::shared_ptr<YUVConversion> conversion) const;

  const std::shared_ptr<YUVConversion>& GetYUVConversion() const;

 private:
  friend SamplerLibrary;

  const vk::Device device_;
  SamplerDescriptor desc_;
  SharedHandleVK<vk::Sampler> sampler_;
  std::shared_ptr<YUVConversion> yuv_conversion_;
  bool mips_disabled_workaround_ = false;
  bool is_valid_ = false;

  Sampler(const Sampler&) = delete;

  Sampler& operator=(const Sampler&) = delete;
};

}  // namespace ogre

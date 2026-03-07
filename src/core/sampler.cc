// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/sampler.h"

namespace ogre {

Sampler::Sampler(const SamplerDescriptor& desc) : desc_(desc) {}

Sampler::~Sampler() = default;

const SamplerDescriptor& Sampler::GetDescriptor() const {
  return desc_;
}

}  // namespace ogre

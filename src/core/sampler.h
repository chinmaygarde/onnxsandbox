// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_CORE_SAMPLER_H_
#define FLUTTER_OGRE_CORE_SAMPLER_H_

#include "core/sampler_descriptor.h"

namespace ogre {

class Sampler {
 public:
  virtual ~Sampler();

  const SamplerDescriptor& GetDescriptor() const;

 protected:
  SamplerDescriptor desc_;

  explicit Sampler(const SamplerDescriptor& desc);

 private:
  Sampler(const Sampler&) = delete;

  Sampler& operator=(const Sampler&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_CORE_SAMPLER_H_

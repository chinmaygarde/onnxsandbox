// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/compute_pass.h"

namespace ogre {

ComputePass::ComputePass(std::shared_ptr<const Context> context)
    : context_(std::move(context)) {}

ComputePass::~ComputePass() = default;

void ComputePass::SetLabel(const std::string& label) {
  if (label.empty()) {
    return;
  }
  OnSetLabel(label);
}

}  // namespace ogre

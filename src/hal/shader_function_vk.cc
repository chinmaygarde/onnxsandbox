// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/shader_function_vk.h"

#include "fml/hash_combine.h"

namespace ogre {

ShaderFunction::ShaderFunction(const std::weak_ptr<DeviceHolder>& device_holder,
                               UniqueID parent_library_id,
                               std::string name,
                               ShaderStage stage,
                               vk::UniqueShaderModule module)
    : parent_library_id_(parent_library_id),
      name_(std::move(name)),
      stage_(stage),
      module_(std::move(module)),
      device_holder_(device_holder) {}

ShaderFunction::~ShaderFunction() {
  std::shared_ptr<DeviceHolder> device_holder = device_holder_.lock();
  if (device_holder) {
    module_.reset();
  } else {
    module_.release();
  }
}

ShaderStage ShaderFunction::GetStage() const {
  return stage_;
}

const std::string& ShaderFunction::GetName() const {
  return name_;
}

const vk::ShaderModule& ShaderFunction::GetModule() const {
  return module_.get();
}

std::size_t ShaderFunction::GetHash() const {
  return fml::HashCombine(parent_library_id_, name_, stage_);
}

bool ShaderFunction::IsEqual(const ShaderFunction& other) const {
  return parent_library_id_ == other.parent_library_id_ &&
         name_ == other.name_ && stage_ == other.stage_;
}

}  // namespace ogre

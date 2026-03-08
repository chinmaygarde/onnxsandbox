// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/compute_pipeline_descriptor.h"

#include <absl/log/check.h>
#include "core/formats.h"
#include "renderer/backend/vulkan/shader_function_vk.h"
#include "renderer/vertex_descriptor.h"

namespace ogre {

ComputePipelineDescriptor::ComputePipelineDescriptor() = default;

ComputePipelineDescriptor::~ComputePipelineDescriptor() = default;

// Comparable<ComputePipelineDescriptor>
std::size_t ComputePipelineDescriptor::GetHash() const {
  auto seed = fml::HashCombine();
  fml::HashCombineSeed(seed, label_);
  if (entrypoint_) {
    fml::HashCombineSeed(seed, entrypoint_->GetHash());
  }
  return seed;
}

// Comparable<ComputePipelineDescriptor>
bool ComputePipelineDescriptor::IsEqual(
    const ComputePipelineDescriptor& other) const {
  return label_ == other.label_ &&
         DeepComparePointer(entrypoint_, other.entrypoint_);
}

ComputePipelineDescriptor& ComputePipelineDescriptor::SetLabel(
    std::string_view label) {
  label_ = label;
  return *this;
}

ComputePipelineDescriptor& ComputePipelineDescriptor::SetStageEntrypoint(
    std::shared_ptr<const ShaderFunction> function) {
  DCHECK(!function || function->GetStage() == ShaderStage::kCompute);
  if (!function || function->GetStage() != ShaderStage::kCompute) {
    return *this;
  }

  if (function->GetStage() == ShaderStage::kUnknown) {
    return *this;
  }

  entrypoint_ = std::move(function);

  return *this;
}

std::shared_ptr<const ShaderFunction>
ComputePipelineDescriptor::GetStageEntrypoint() const {
  return entrypoint_;
}

const std::string& ComputePipelineDescriptor::GetLabel() const {
  return label_;
}

bool ComputePipelineDescriptor::RegisterDescriptorSetLayouts(
    const DescriptorSetLayout desc_set_layout[],
    size_t count) {
  descriptor_set_layouts_.reserve(descriptor_set_layouts_.size() + count);
  for (size_t i = 0; i < count; i++) {
    descriptor_set_layouts_.emplace_back(desc_set_layout[i]);
  }
  return true;
}

const std::vector<DescriptorSetLayout>&
ComputePipelineDescriptor::GetDescriptorSetLayouts() const {
  return descriptor_set_layouts_;
}

}  // namespace ogre

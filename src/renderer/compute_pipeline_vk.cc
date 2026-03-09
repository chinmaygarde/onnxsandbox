// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/compute_pipeline_vk.h"

namespace ogre {

ComputePipeline::ComputePipeline(
    std::weak_ptr<DeviceHolder> device_holder,
    std::weak_ptr<PipelineLibrary> library,
    const ComputePipelineDescriptor& desc,
    vk::UniquePipeline pipeline,
    vk::UniquePipelineLayout layout,
    vk::UniqueDescriptorSetLayout descriptor_set_layout,
    PipelineKey pipeline_key)
    : Pipeline(std::move(library), desc),
      device_holder_(std::move(device_holder)),
      pipeline_(std::move(pipeline)),
      layout_(std::move(layout)),
      descriptor_set_layout_(std::move(descriptor_set_layout)),
      pipeline_key_(pipeline_key) {
  is_valid_ = pipeline_ && layout_ && descriptor_set_layout_;
}

ComputePipeline::~ComputePipeline() {
  std::shared_ptr<DeviceHolder> device_holder = device_holder_.lock();
  if (device_holder) {
    descriptor_set_layout_.reset();
    layout_.reset();
    pipeline_.reset();
  } else {
    descriptor_set_layout_.release();
    layout_.release();
    pipeline_.release();
  }
}

bool ComputePipeline::IsValid() const {
  return is_valid_;
}

const vk::Pipeline& ComputePipeline::GetPipeline() const {
  return *pipeline_;
}

const vk::PipelineLayout& ComputePipeline::GetPipelineLayout() const {
  return *layout_;
}

const vk::DescriptorSetLayout& ComputePipeline::GetDescriptorSetLayout() const {
  return *descriptor_set_layout_;
}

PipelineKey ComputePipeline::GetPipelineKey() const {
  return pipeline_key_;
}

}  // namespace ogre

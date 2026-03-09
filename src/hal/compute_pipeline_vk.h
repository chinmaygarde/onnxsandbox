// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/backend_cast.h"
#include "hal/device_holder_vk.h"
#include "hal/pipeline.h"
#include "hal/vk.h"

namespace ogre {

class PipelineLibrary;

class ComputePipeline final
    : public Pipeline<ComputePipelineDescriptor>,
      public BackendCast<ComputePipeline, Pipeline<ComputePipelineDescriptor>> {
 public:
  ComputePipeline(std::weak_ptr<DeviceHolder> device_holder,
                  std::weak_ptr<PipelineLibrary> library,
                  const ComputePipelineDescriptor& desc,
                  vk::UniquePipeline pipeline,
                  vk::UniquePipelineLayout layout,
                  vk::UniqueDescriptorSetLayout descriptor_set_layout,
                  PipelineKey pipeline_key);

  // |Pipeline|
  ~ComputePipeline() override;

  const vk::Pipeline& GetPipeline() const;

  const vk::PipelineLayout& GetPipelineLayout() const;

  const vk::DescriptorSetLayout& GetDescriptorSetLayout() const;

  /// @brief Retrieve the unique identifier for this pipeline's descriptor set
  ///        layout.
  PipelineKey GetPipelineKey() const;

 private:
  friend class PipelineLibrary;

  std::weak_ptr<DeviceHolder> device_holder_;
  vk::UniquePipeline pipeline_;
  vk::UniquePipelineLayout layout_;
  vk::UniqueDescriptorSetLayout descriptor_set_layout_;
  const PipelineKey pipeline_key_;
  bool is_valid_ = false;

  // |Pipeline|
  bool IsValid() const override;

  ComputePipeline(const ComputePipeline&) = delete;

  ComputePipeline& operator=(const ComputePipeline&) = delete;
};

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMPUTE_PASS_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMPUTE_PASS_VK_H_

#include <memory>
#include <string>

#include "fml/status.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/pipeline_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "renderer/resource_binder.h"

namespace ogre {

class CommandBuffer;

class ComputePass final : public ResourceBinder {
 public:
  ~ComputePass();

  bool IsValid() const;

  void SetLabel(const std::string& label);

  void SetCommandLabel(std::string_view label);

  void SetPipeline(
      const std::shared_ptr<Pipeline<ComputePipelineDescriptor>>& pipeline);

  fml::Status Compute(const ISize& grid_size);

  void AddBufferMemoryBarrier();

  void AddTextureMemoryBarrier();

  bool EncodeCommands() const;

  const Context& GetContext() const { return *context_; }

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    DescriptorType type,
                    const ShaderUniformSlot& slot,
                    const ShaderMetadata* metadata,
                    BufferView view) override;

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    DescriptorType type,
                    const SampledImageSlot& slot,
                    const ShaderMetadata* metadata,
                    std::shared_ptr<const Texture> texture,
                    raw_ptr<const Sampler> sampler) override;

  bool BindResource(size_t binding, DescriptorType type, BufferView view);

 private:
  friend class CommandBuffer;

  const std::shared_ptr<const Context> context_;
  std::shared_ptr<CommandBuffer> command_buffer_;
  std::string label_;
  std::array<uint32_t, 3> max_wg_size_ = {};
  bool is_valid_ = false;

  // Per-command state.
  std::array<vk::DescriptorImageInfo, kMaxBindings> image_workspace_;
  std::array<vk::DescriptorBufferInfo, kMaxBindings> buffer_workspace_;
  std::array<vk::WriteDescriptorSet, kMaxBindings + kMaxBindings>
      write_workspace_;
  size_t bound_image_offset_ = 0u;
  size_t bound_buffer_offset_ = 0u;
  size_t descriptor_write_offset_ = 0u;
  bool has_label_ = false;
  bool pipeline_valid_ = false;
  vk::DescriptorSet descriptor_set_ = {};
  vk::PipelineLayout pipeline_layout_ = {};

  ComputePass(std::shared_ptr<const Context> context,
              std::shared_ptr<CommandBuffer> command_buffer);

  void OnSetLabel(const std::string& label);

  ComputePass(const ComputePass&) = delete;

  ComputePass& operator=(const ComputePass&) = delete;
};

}  // namespace ogre
#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_COMPUTE_PASS_VK_H_

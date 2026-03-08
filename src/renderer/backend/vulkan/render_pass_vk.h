// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_RENDER_PASS_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_RENDER_PASS_VK_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/buffer_view.h"
#include "core/formats.h"
#include "core/shader_types.h"
#include "core/vertex_buffer.h"
#include "fml/status.h"
#include "geometry/matrix.h"
#include "geometry/size.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/pipeline_vk.h"
#include "renderer/backend/vulkan/shared_object_vk.h"
#include "renderer/command.h"
#include "renderer/render_target.h"
#include "renderer/resource_binder.h"

namespace ogre {

class CommandBuffer;
class Context;
class Sampler;

class RenderPass final : public ResourceBinder {
 public:
  ~RenderPass();

  const std::shared_ptr<const Context>& GetContext() const;

  const RenderTarget& GetRenderTarget() const;

  ISize GetRenderTargetSize() const;

  const Matrix& GetOrthographicTransform() const;

  bool IsValid() const;

  void SetLabel(std::string_view label);

  void SetPipeline(PipelineRef pipeline);

  void SetPipeline(
      const std::shared_ptr<Pipeline<PipelineDescriptor>>& pipeline);

  void SetCommandLabel(std::string_view label);

  void SetStencilReference(uint32_t value);

  void SetBaseVertex(uint64_t value);

  void SetViewport(Viewport viewport);

  void SetScissor(IRect32 scissor);

  void SetElementCount(size_t count);

  void SetInstanceCount(size_t count);

  bool SetVertexBuffer(VertexBuffer buffer);

  bool SetVertexBuffer(BufferView vertex_buffer);

  bool SetVertexBuffer(std::vector<BufferView> vertex_buffers);

  bool SetVertexBuffer(BufferView vertex_buffers[], size_t vertex_buffer_count);

  bool SetIndexBuffer(BufferView index_buffer, IndexType index_type);

  fml::Status Draw();

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
                    raw_ptr<const Sampler>) override;

  bool BindDynamicResource(ShaderStage stage,
                           DescriptorType type,
                           const SampledImageSlot& slot,
                           std::unique_ptr<ShaderMetadata> metadata,
                           std::shared_ptr<const Texture> texture,
                           raw_ptr<const Sampler>);

  bool BindDynamicResource(ShaderStage stage,
                           DescriptorType type,
                           const ShaderUniformSlot& slot,
                           std::unique_ptr<ShaderMetadata> metadata,
                           BufferView view);

  bool EncodeCommands() const;

  SampleCount GetSampleCount() const;

  PixelFormat GetRenderTargetPixelFormat() const;

  bool HasDepthAttachment() const;

  bool HasStencilAttachment() const;

 private:
  friend class CommandBuffer;

  const std::shared_ptr<const Context> context_;
  const SampleCount sample_count_;
  const PixelFormat pixel_format_;
  const bool has_depth_attachment_;
  const bool has_stencil_attachment_;
  const ISize render_target_size_;
  RenderTarget render_target_;
  const Matrix orthographic_;
  std::shared_ptr<CommandBuffer> command_buffer_;
  std::string debug_label_;
  SharedHandleVK<vk::RenderPass> render_pass_;
  bool is_valid_ = false;

  vk::CommandBuffer command_buffer_vk_;
  std::shared_ptr<Texture> color_image_vk_;
  std::shared_ptr<Texture> resolve_image_vk_;
  uint32_t current_stencil_ = 0;

  // Per-command state.
  std::array<vk::DescriptorImageInfo, kMaxBindings> image_workspace_;
  std::array<vk::DescriptorBufferInfo, kMaxBindings> buffer_workspace_;
  std::array<vk::WriteDescriptorSet, kMaxBindings + kMaxBindings>
      write_workspace_;
  size_t bound_image_offset_ = 0u;
  size_t bound_buffer_offset_ = 0u;
  size_t descriptor_write_offset_ = 0u;
  size_t instance_count_ = 1u;
  size_t base_vertex_ = 0u;
  size_t element_count_ = 0u;
  bool has_index_buffer_ = false;
  bool has_label_ = false;
  PipelineRef pipeline_ = PipelineRef(nullptr);
  bool pipeline_uses_input_attachments_ = false;
  std::shared_ptr<Sampler> immutable_sampler_;

  RenderPass(const std::shared_ptr<const Context>& context,
             const RenderTarget& target,
             std::shared_ptr<CommandBuffer> command_buffer);

  static bool ValidateVertexBuffers(const BufferView vertex_buffers[],
                                    size_t vertex_buffer_count);

  static bool ValidateIndexBuffer(const BufferView& index_buffer,
                                  IndexType index_type);

  void OnSetLabel(std::string_view label);

  bool OnEncodeCommands(const Context& context) const;

  bool BindResource(size_t binding, DescriptorType type, BufferView view);

  SharedHandleVK<vk::RenderPass> CreateVKRenderPass(
      const Context& context,
      const SharedHandleVK<vk::RenderPass>& recycled_renderpass,
      const std::shared_ptr<CommandBuffer>& command_buffer,
      bool is_swapchain) const;

  SharedHandleVK<vk::Framebuffer> CreateVKFramebuffer(
      const Context& context,
      const vk::RenderPass& pass) const;

  RenderPass(const RenderPass&) = delete;

  RenderPass& operator=(const RenderPass&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_RENDER_PASS_VK_H_

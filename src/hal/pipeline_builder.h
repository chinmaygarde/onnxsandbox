// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <format>

#include "core/formats.h"
#include "hal/capabilities_vk.h"
#include "hal/context_vk.h"
#include "hal/pipeline_descriptor.h"
#include "hal/shader_library_vk.h"
#include "hal/vertex_descriptor.h"

namespace ogre {

//------------------------------------------------------------------------------
/// @brief      An optional (but highly recommended) utility for creating
///             pipelines from reflected shader information.
///
/// @tparam     VertexShader_    The reflected vertex shader information. Found
///                              in a generated header file called
///                              <shader_name>.vert.h.
/// @tparam     FragmentShader_  The reflected fragment shader information.
///                              Found in a generated header file called
///                              <shader_name>.frag.h.
///
template <class VertexShader_, class FragmentShader_>
struct PipelineBuilder {
 public:
  using VertexShader = VertexShader_;
  using FragmentShader = FragmentShader_;

  static constexpr size_t kVertexBufferIndex =
      VertexDescriptor::kReservedVertexBufferIndex;

  //----------------------------------------------------------------------------
  /// @brief      Create a default pipeline descriptor using the combination
  ///             reflected shader information. The descriptor can be configured
  ///             further before a pipeline state object is created using it.
  ///
  /// @param[in]  context  The context
  ///
  /// @return     If the combination of reflected shader information is
  ///             compatible and the requisite functions can be found in the
  ///             context, a pipeline descriptor.
  ///
  static std::optional<PipelineDescriptor> MakeDefaultPipelineDescriptor(
      const Context& context,
      const std::vector<Scalar>& constants = {}) {
    PipelineDescriptor desc;
    desc.SetSpecializationConstants(constants);
    if (InitializePipelineDescriptorDefaults(context, desc)) {
      return {std::move(desc)};
    }
    return std::nullopt;
  }

  [[nodiscard]] static bool InitializePipelineDescriptorDefaults(
      const Context& context,
      PipelineDescriptor& desc) {
    // Setup debug instrumentation.
    desc.SetLabel(std::format("{} Pipeline", FragmentShader::kLabel));

    // Resolve pipeline entrypoints.
    {
      auto vertex_function = context.GetShaderLibrary()->GetFunction(
          VertexShader::kEntrypointName, ShaderStage::kVertex);
      auto fragment_function = context.GetShaderLibrary()->GetFunction(
          FragmentShader::kEntrypointName, ShaderStage::kFragment);

      if (!vertex_function || !fragment_function) {
        LOG(ERROR) << "Could not resolve pipeline entrypoint(s) '"
                   << VertexShader::kEntrypointName << "' and '"
                   << FragmentShader::kEntrypointName
                   << "' for pipeline named '" << VertexShader::kLabel << "'.";
        return false;
      }

      desc.AddStageEntrypoint(std::move(vertex_function));
      desc.AddStageEntrypoint(std::move(fragment_function));
    }

    // Setup the vertex descriptor from reflected information.
    {
      auto vertex_descriptor = std::make_shared<VertexDescriptor>();
      vertex_descriptor->SetStageInputs(VertexShader::kAllShaderStageInputs,
                                        VertexShader::kInterleavedBufferLayout);
      vertex_descriptor->RegisterDescriptorSetLayouts(
          VertexShader::kDescriptorSetLayouts);
      vertex_descriptor->RegisterDescriptorSetLayouts(
          FragmentShader::kDescriptorSetLayouts);
      desc.SetVertexDescriptor(std::move(vertex_descriptor));
    }

    // Setup fragment shader output descriptions.
    {
      // Configure the sole color attachments pixel format. This is by
      // convention.
      ColorAttachmentDescriptor color0;
      color0.format = context.GetCapabilities()->GetDefaultColorFormat();
      color0.blending_enabled = true;
      desc.SetColorAttachmentDescriptor(0u, color0);
    }

    // Setup default depth buffer descriptions.
    {
      DepthAttachmentDescriptor depth0;
      depth0.depth_compare = CompareFunction::kAlways;
      desc.SetDepthStencilAttachmentDescriptor(depth0);
      desc.SetDepthPixelFormat(
          context.GetCapabilities()->GetDefaultDepthStencilFormat());
    }

    // Setup default stencil buffer descriptions.
    {
      StencilAttachmentDescriptor stencil0;
      stencil0.stencil_compare = CompareFunction::kEqual;
      desc.SetStencilAttachmentDescriptors(stencil0);
      desc.SetStencilPixelFormat(
          context.GetCapabilities()->GetDefaultDepthStencilFormat());
    }

    return true;
  }
};

}  // namespace ogre

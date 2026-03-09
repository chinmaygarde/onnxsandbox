// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <format>

#include "hal/compute_pipeline_descriptor.h"
#include "hal/context_vk.h"
#include "hal/shader_library_vk.h"

namespace ogre {

//------------------------------------------------------------------------------
/// @brief      An optional (but highly recommended) utility for creating
///             pipelines from reflected shader information.
///
/// @tparam     Compute_Shader   The reflected compute shader information. Found
///                              in a generated header file called
///                              <shader_name>.comp.h.
///
template <class ComputeShader_>
struct ComputePipelineBuilder {
 public:
  using ComputeShader = ComputeShader_;

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
  static std::optional<ComputePipelineDescriptor> MakeDefaultPipelineDescriptor(
      const Context& context) {
    ComputePipelineDescriptor desc;
    if (InitializePipelineDescriptorDefaults(context, desc)) {
      return {std::move(desc)};
    }
    return std::nullopt;
  }

  [[nodiscard]] static bool InitializePipelineDescriptorDefaults(
      const Context& context,
      ComputePipelineDescriptor& desc) {
    // Setup debug instrumentation.
    desc.SetLabel(std::format("{} Pipeline", ComputeShader::kLabel));

    // Resolve pipeline entrypoints.
    {
      auto compute_function = context.GetShaderLibrary()->GetFunction(
          ComputeShader::kEntrypointName, ShaderStage::kCompute);

      if (!compute_function) {
        LOG(ERROR) << "Could not resolve compute pipeline entrypoint '"
                   << ComputeShader::kEntrypointName << "' for pipeline named '"
                   << ComputeShader::kLabel << "'.";
        return false;
      }

      if (!desc.RegisterDescriptorSetLayouts(
              ComputeShader::kDescriptorSetLayouts)) {
        LOG(ERROR) << "Could not configure compute descriptor set layout "
                      "for pipeline named '"
                   << ComputeShader::kLabel << "'.";
        return false;
      }

      desc.SetStageEntrypoint(std::move(compute_function));
    }
    return true;
  }
};

}  // namespace ogre

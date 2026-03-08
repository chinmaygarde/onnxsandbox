// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/comparable.h"
#include "core/shader_types.h"
#include "renderer/backend/vulkan/device_holder_vk.h"
#include "renderer/backend/vulkan/vk.h"

namespace ogre {

class ShaderFunction final : public Comparable<ShaderFunction> {
 public:
  ~ShaderFunction();

  ShaderStage GetStage() const;

  const std::string& GetName() const;

  const vk::ShaderModule& GetModule() const;

  // |Comparable<ShaderFunction>|
  std::size_t GetHash() const override;

  // |Comparable<ShaderFunction>|
  bool IsEqual(const ShaderFunction& other) const override;

 private:
  friend class ShaderLibrary;

  UniqueID parent_library_id_;
  std::string name_;
  ShaderStage stage_;
  vk::UniqueShaderModule module_;
  std::weak_ptr<DeviceHolder> device_holder_;

  ShaderFunction(const std::weak_ptr<DeviceHolder>& device_holder,
                 UniqueID parent_library_id,
                 std::string name,
                 ShaderStage stage,
                 vk::UniqueShaderModule module);

  ShaderFunction(const ShaderFunction&) = delete;

  ShaderFunction& operator=(const ShaderFunction&) = delete;
};

}  // namespace ogre

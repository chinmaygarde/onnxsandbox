// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/thread.h"
#include "core/shader_types.h"
#include "fml/mapping.h"
#include "hal/device_holder_vk.h"
#include "hal/shader_function_vk.h"
#include "hal/shader_key.h"
#include "hal/vk.h"

namespace ogre {

class ShaderLibrary final : public std::enable_shared_from_this<ShaderLibrary> {
 public:
  ~ShaderLibrary();

  bool IsValid() const;

  std::shared_ptr<const ShaderFunction> GetFunction(std::string_view name,
                                                    ShaderStage stage);

  using RegistrationCallback = std::function<void(bool)>;
  void RegisterFunction(std::string name,
                        ShaderStage stage,
                        std::shared_ptr<fml::Mapping> code,
                        RegistrationCallback callback);

  void UnregisterFunction(std::string name, ShaderStage stage);

 private:
  friend class Context;
  std::weak_ptr<DeviceHolder> device_holder_;
  const UniqueID library_id_;
  mutable RWMutex functions_mutex_;
  ShaderFunctionMap functions_ IPLR_GUARDED_BY(functions_mutex_);
  bool is_valid_ = false;

  ShaderLibrary(
      std::weak_ptr<DeviceHolder> device_holder,
      const std::vector<std::shared_ptr<fml::Mapping>>& shader_libraries_data);

  bool RegisterFunction(const std::string& name,
                        ShaderStage stage,
                        const std::shared_ptr<fml::Mapping>& code);

  ShaderLibrary(const ShaderLibrary&) = delete;

  ShaderLibrary& operator=(const ShaderLibrary&) = delete;
};

}  // namespace ogre

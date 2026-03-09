// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/shader_library_vk.h"

#include <cstdint>

#include <absl/log/check.h>

#include "fml/trace_event.h"
#include "renderer/context_vk.h"
#include "renderer/shader_function_vk.h"

namespace ogre {

ShaderLibrary::ShaderLibrary(
    std::weak_ptr<DeviceHolder> device_holder,
    const std::vector<std::shared_ptr<fml::Mapping>>& shader_libraries_data)
    : device_holder_(std::move(device_holder)) {
  TRACE_EVENT0("ogre", "CreateShaderLibrary");
}

ShaderLibrary::~ShaderLibrary() = default;

bool ShaderLibrary::IsValid() const {
  return is_valid_;
}

std::shared_ptr<const ShaderFunction> ShaderLibrary::GetFunction(
    std::string_view name,
    ShaderStage stage) {
  ReaderLock lock(functions_mutex_);

  const auto key = ShaderKey{{name.data(), name.size()}, stage};
  auto found = functions_.find(key);
  if (found != functions_.end()) {
    return found->second;
  }
  return nullptr;
}

void ShaderLibrary::RegisterFunction(std::string name,
                                     ShaderStage stage,
                                     std::shared_ptr<fml::Mapping> code,
                                     RegistrationCallback callback) {
  const auto result = RegisterFunction(name, stage, code);
  if (callback) {
    callback(result);
  }
}

static bool IsMappingSPIRV(const fml::Mapping& mapping) {
  // https://registry.khronos.org/SPIR-V/specs/1.0/SPIRV.html#Magic
  const uint32_t kSPIRVMagic = 0x07230203;
  if (mapping.GetSize() < sizeof(kSPIRVMagic)) {
    return false;
  }
  uint32_t magic = 0u;
  ::memcpy(&magic, mapping.GetMapping(), sizeof(magic));
  return magic == kSPIRVMagic;
}

bool ShaderLibrary::RegisterFunction(
    const std::string& name,
    ShaderStage stage,
    const std::shared_ptr<fml::Mapping>& code) {
  if (!code) {
    return false;
  }

  if (!IsMappingSPIRV(*code)) {
    LOG(ERROR) << "Shader is not valid SPIRV.";
    return false;
  }

  vk::ShaderModuleCreateInfo shader_module_info;

  shader_module_info.setPCode(
      reinterpret_cast<const uint32_t*>(code->GetMapping()));
  shader_module_info.setCodeSize(code->GetSize());

  auto device_holder = device_holder_.lock();
  if (!device_holder) {
    return false;
  }
  DCHECK(device_holder->GetDevice());
  auto module =
      device_holder->GetDevice().createShaderModuleUnique(shader_module_info);

  if (module.result != vk::Result::eSuccess) {
    LOG(ERROR) << "Could not create shader module: "
               << vk::to_string(module.result);
    return false;
  }

  vk::UniqueShaderModule shader_module = std::move(module.value);
  Context::SetDebugName(device_holder->GetDevice(), *shader_module,
                        "Shader " + name);

  WriterLock lock(functions_mutex_);
  functions_[ShaderKey{name, stage}] = std::shared_ptr<ShaderFunction>(
      new ShaderFunction(device_holder_,
                         library_id_,              //
                         name,                     //
                         stage,                    //
                         std::move(shader_module)  //
                         ));

  return true;
}

void ShaderLibrary::UnregisterFunction(std::string name, ShaderStage stage) {
  WriterLock lock(functions_mutex_);

  const auto key = ShaderKey{name, stage};

  auto found = functions_.find(key);
  if (found == functions_.end()) {
    LOG(ERROR) << "Library function named " << name
               << " was not found, so it couldn't be unregistered.";
    return;
  }

  functions_.erase(found);

  return;
}

}  // namespace ogre

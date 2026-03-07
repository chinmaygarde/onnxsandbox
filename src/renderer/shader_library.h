// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_SHADER_LIBRARY_H_
#define FLUTTER_OGRE_RENDERER_SHADER_LIBRARY_H_

#include <future>
#include <memory>
#include <string_view>

#include "core/shader_types.h"
#include "fml/mapping.h"

namespace ogre {

class Context;
class ShaderFunction;

class ShaderLibrary : public std::enable_shared_from_this<ShaderLibrary> {
 public:
  virtual ~ShaderLibrary();

  virtual bool IsValid() const = 0;

  virtual std::shared_ptr<const ShaderFunction> GetFunction(
      std::string_view name,
      ShaderStage stage) = 0;

  using RegistrationCallback = std::function<void(bool)>;
  virtual void RegisterFunction(std::string name,
                                ShaderStage stage,
                                std::shared_ptr<fml::Mapping> code,
                                RegistrationCallback callback);

  virtual void UnregisterFunction(std::string name, ShaderStage stage) = 0;

 protected:
  ShaderLibrary();

 private:
  ShaderLibrary(const ShaderLibrary&) = delete;

  ShaderLibrary& operator=(const ShaderLibrary&) = delete;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_SHADER_LIBRARY_H_

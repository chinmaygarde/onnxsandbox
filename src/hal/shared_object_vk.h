// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "hal/vk.h"

namespace ogre {

class SharedObject {
 public:
  virtual ~SharedObject() = default;
};

template <class T>
class SharedObjectT : public SharedObject {
 public:
  using Resource = T;
  using UniqueResource =
      vk::UniqueHandle<Resource, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;

  explicit SharedObjectT(UniqueResource res) : resource_(std::move(res)) {}

  // NOLINTNEXTLINE(google-explicit-constructor)
  operator Resource() const { return Get(); }

  const Resource& Get() const { return *resource_; }

  const UniqueResource& GetUniqueWrapper() const { return resource_; }

 private:
  UniqueResource resource_;

  SharedObjectT(const SharedObjectT&) = delete;

  SharedObjectT& operator=(const SharedObjectT&) = delete;
};

template <class T>
auto MakeSharedVK(
    vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> handle) {
  if (!handle) {
    return std::shared_ptr<SharedObjectT<T>>{nullptr};
  }
  return std::make_shared<SharedObjectT<T>>(std::move(handle));
}

template <class T>
using SharedHandleVK = std::shared_ptr<SharedObjectT<T>>;

}  // namespace ogre

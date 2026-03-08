// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "renderer/backend/vulkan/vk.h"

namespace ogre {

class Capabilities;

class DebugReport {
 public:
  DebugReport(const Capabilities& caps, const vk::Instance& instance);

  ~DebugReport();

  bool IsValid() const;

 private:
  vk::UniqueDebugUtilsMessengerEXT messenger_;
  bool is_valid_ = false;

  enum class Result {
    kContinue,
    kAbort,
  };

  Result OnDebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                         vk::DebugUtilsMessageTypeFlagsEXT type,
                         const vk::DebugUtilsMessengerCallbackDataEXT* data);

  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
      void* user_data);

  DebugReport(const DebugReport&) = delete;

  DebugReport& operator=(const DebugReport&) = delete;
};

}  // namespace ogre

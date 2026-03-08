// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_EXTERNAL_SEMAPHORE_VK_H_
#define FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_EXTERNAL_SEMAPHORE_VK_H_

#include "fml/unique_fd.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/shared_object_vk.h"
#include "renderer/backend/vulkan/vk.h"
#include "vulkan/vulkan_handles.hpp"

namespace ogre {

//------------------------------------------------------------------------------
/// @brief      A Vulkan semaphore that can be exported as a platform specific
///             file descriptor.
///
///             The semaphore are exported as sync file descriptors.
///
/// @warning    Only semaphore that have been signaled or have a single
///             operation pending can be exported. Make sure to submit a fence
///             signalling operation to a queue before attempted to obtain a
///             file descriptor for the fence.
///
class ExternalSemaphore {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Create a new un-signaled semaphore that can be exported as a
  ///             sync file descriptor.
  ///
  /// @param[in]  context  The device context.
  ///
  explicit ExternalSemaphore(const std::shared_ptr<Context>& context);

  ~ExternalSemaphore();

  ExternalSemaphore(const ExternalSemaphore&) = delete;

  ExternalSemaphore& operator=(const ExternalSemaphore&) = delete;

  //----------------------------------------------------------------------------
  /// @brief      If a valid fence could be created.
  ///
  /// @return     True if valid, False otherwise.
  ///
  bool IsValid() const;

  //----------------------------------------------------------------------------
  /// @brief      Create a new sync file descriptor for the underlying
  ///             semaphore.
  ///
  ///             The semaphore must already be signaled or have a signal
  ///             operation pending in a queue. There are no checks for this in
  ///             the implementation and only Vulkan validation will catch such
  ///             a misuse and undefined behavior.
  ///
  /// @warning    Implementations are also allowed to return invalid file
  ///             descriptors in case a semaphore has already been signaled. So
  ///             it is not necessary an error to obtain an invalid descriptor
  ///             from this call. For APIs that are meant to consume such
  ///             descriptors, pass -1 as the file handle.
  ///
  ///             Since this call can return an invalid FD even in case of
  ///             success, make sure to make the `IsValid` check before
  ///             attempting to export a FD.
  ///
  /// @return     A (potentially invalid even in case of success) file
  ///             descriptor.
  ///
  fml::UniqueFD CreateFD() const;

  const vk::Semaphore& GetHandle() const;

  const SharedHandleVK<vk::Semaphore>& GetSharedHandle() const;

 private:
  SharedHandleVK<vk::Semaphore> semaphore_;
};

}  // namespace ogre

#endif  // FLUTTER_OGRE_RENDERER_BACKEND_VULKAN_SWAPCHAIN_AHB_EXTERNAL_SEMAPHORE_VK_H_

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/backend/vulkan/surface_context_vk.h"

#include "core/runtime_types.h"
#include "fml/trace_event.h"
#include "renderer/backend/vulkan/allocator_vk.h"
#include "renderer/backend/vulkan/command_pool_vk.h"
#include "renderer/backend/vulkan/context_vk.h"
#include "renderer/backend/vulkan/swapchain/khr/khr_swapchain_vk.h"

namespace ogre {

SurfaceContext::SurfaceContext(const std::shared_ptr<ContextVK>& parent)
    : Context(parent->GetFlags()), parent_(parent) {}

SurfaceContext::~SurfaceContext() = default;

Context::BackendType SurfaceContext::GetBackendType() const {
  return parent_->GetBackendType();
}

std::string SurfaceContext::DescribeGpuModel() const {
  return parent_->DescribeGpuModel();
}

bool SurfaceContext::IsValid() const {
  return parent_->IsValid();
}

std::shared_ptr<Allocator> SurfaceContext::GetResourceAllocator() const {
  return parent_->GetResourceAllocator();
}

std::shared_ptr<ShaderLibraryVK> SurfaceContext::GetShaderLibrary() const {
  return parent_->GetShaderLibrary();
}

std::shared_ptr<SamplerLibraryVK> SurfaceContext::GetSamplerLibrary() const {
  return parent_->GetSamplerLibrary();
}

std::shared_ptr<PipelineLibraryVK> SurfaceContext::GetPipelineLibrary() const {
  return parent_->GetPipelineLibrary();
}

std::shared_ptr<CommandBuffer> SurfaceContext::CreateCommandBuffer() const {
  return parent_->CreateCommandBuffer();
}

std::shared_ptr<CommandQueue> SurfaceContext::GetCommandQueue() const {
  return parent_->GetCommandQueue();
}

const std::shared_ptr<const Capabilities>& SurfaceContext::GetCapabilities()
    const {
  return parent_->GetCapabilities();
}

std::shared_ptr<const IdleWaiterVK> SurfaceContext::GetIdleWaiter() const {
  return parent_->GetIdleWaiter();
}

void SurfaceContext::Shutdown() {
  parent_->Shutdown();
}

bool SurfaceContext::SetWindowSurface(vk::UniqueSurfaceKHR surface,
                                      const ISize& size) {
  return SetSwapchain(Swapchain::Create(parent_, std::move(surface), size));
}

void SurfaceContext::TeardownSwapchain() {
  // When background the application, tear down the swapchain to release memory
  // from the images. When returning to the foreground, SetWindowSurface will be
  // called which will re-create the swapchain.
  swapchain_.reset();
}

bool SurfaceContext::SetSwapchain(std::shared_ptr<Swapchain> swapchain) {
  if (!swapchain || !swapchain->IsValid()) {
    VALIDATION_LOG << "Invalid swapchain.";
    return false;
  }
  swapchain_ = std::move(swapchain);
  return true;
}

std::unique_ptr<SurfaceVK> SurfaceContext::AcquireNextSurface() {
  TRACE_EVENT0("ogre", __FUNCTION__);
  auto surface = swapchain_ ? swapchain_->AcquireNextDrawable() : nullptr;
  if (!surface) {
    return nullptr;
  }
  MarkFrameEnd();
  return surface;
}

void SurfaceContext::MarkFrameEnd() {
  if (auto pipeline_library = parent_->GetPipelineLibrary()) {
    pipeline_library->DidAcquireSurfaceFrame();
  }
  parent_->DisposeThreadLocalCachedResources();
  parent_->GetResourceAllocator()->DebugTraceMemoryStatistics();
}

void SurfaceContext::UpdateSurfaceSize(const ISize& size) const {
  swapchain_->UpdateSurfaceSize(size);
}

const vk::Device& SurfaceContext::GetDevice() const {
  return parent_->GetDevice();
}

void SurfaceContext::InitializeCommonlyUsedShadersIfNeeded() const {
  parent_->InitializeCommonlyUsedShadersIfNeeded();
}

void SurfaceContext::DisposeThreadLocalCachedResources() {
  parent_->DisposeThreadLocalCachedResources();
}

const std::shared_ptr<ContextVK>& SurfaceContext::GetParent() const {
  return parent_;
}

bool SurfaceContext::EnqueueCommandBuffer(
    std::shared_ptr<CommandBuffer> command_buffer) {
  return parent_->EnqueueCommandBuffer(std::move(command_buffer));
}

bool SurfaceContext::FlushCommandBuffers() {
  return parent_->FlushCommandBuffers();
}

bool SurfaceContext::SubmitOnscreen(std::shared_ptr<CommandBuffer> cmd_buffer) {
  swapchain_->AddFinalCommandBuffer(std::move(cmd_buffer));
  return true;
}

RuntimeStageBackend SurfaceContext::GetRuntimeStageBackend() const {
  return parent_->GetRuntimeStageBackend();
}

}  // namespace ogre

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hal/pipeline.h"
#include <optional>

#include "base/promise.h"
#include "compute_pipeline_descriptor.h"
#include "hal/compute_pipeline_descriptor.h"
#include "hal/context_vk.h"
#include "hal/pipeline_library_vk.h"
#include "pipeline_descriptor.h"

namespace ogre {

template <typename T>
Pipeline<T>::Pipeline(std::weak_ptr<PipelineLibrary> library, const T& desc)
    : library_(std::move(library)), desc_(std::move(desc)) {}

template <typename T>
Pipeline<T>::~Pipeline() = default;

PipelineFuture<PipelineDescriptor> CreatePipelineFuture(
    const Context& context,
    std::optional<PipelineDescriptor> desc,
    bool async) {
  if (!context.IsValid()) {
    return {desc, RealizedFuture<std::shared_ptr<Pipeline<PipelineDescriptor>>>(
                      nullptr)};
  }

  return context.GetPipelineLibrary()->GetPipeline(std::move(desc),
                                                   /*async=*/async);
}

PipelineFuture<ComputePipelineDescriptor> CreatePipelineFuture(
    const Context& context,
    std::optional<ComputePipelineDescriptor> desc) {
  if (!context.IsValid()) {
    return {
        desc,
        RealizedFuture<std::shared_ptr<Pipeline<ComputePipelineDescriptor>>>(
            nullptr)};
  }

  return context.GetPipelineLibrary()->GetPipeline(std::move(desc));
}

template <typename T>
const T& Pipeline<T>::GetDescriptor() const {
  return desc_;
}

template <typename T>
PipelineFuture<T> Pipeline<T>::CreateVariant(
    bool async,
    const std::function<void(T& desc)>& descriptor_callback) const {
  if (!descriptor_callback) {
    return {std::nullopt,
            RealizedFuture<std::shared_ptr<Pipeline<T>>>(nullptr)};
  }

  auto copied_desc = desc_;

  descriptor_callback(copied_desc);

  auto library = library_.lock();
  if (!library) {
    LOG(ERROR) << "The library from which this pipeline was created was "
                  "already collected.";
    return {desc_, RealizedFuture<std::shared_ptr<Pipeline<T>>>(nullptr)};
  }

  return library->GetPipeline(std::move(copied_desc), async);
}

template class Pipeline<PipelineDescriptor>;
template class Pipeline<ComputePipelineDescriptor>;

}  // namespace ogre

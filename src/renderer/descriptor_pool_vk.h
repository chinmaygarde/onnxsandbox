// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <unordered_map>

#include "fml/status_or.h"
#include "renderer/context_vk.h"
#include "renderer/pipeline.h"

namespace ogre {

/// Used and un-used descriptor sets.
struct DescriptorCache {
  std::vector<vk::DescriptorSet> unused;
  std::vector<vk::DescriptorSet> used;
};

using DescriptorCacheMap = std::unordered_map<PipelineKey, DescriptorCache>;

//------------------------------------------------------------------------------
/// @brief      A per-frame descriptor pool. Descriptors
///             from this pool don't need to be freed individually. Instead, the
///             pool must be collected after all the descriptors allocated from
///             it are done being used.
///
///             The pool or it's descriptors may not be accessed from multiple
///             threads.
///
///             Encoders create pools as necessary as they have the same
///             threading and lifecycle restrictions.
class DescriptorPool {
 public:
  explicit DescriptorPool(std::weak_ptr<const Context> context);

  DescriptorPool(std::weak_ptr<const Context> context,
                 DescriptorCacheMap descriptor_sets,
                 std::vector<vk::UniqueDescriptorPool> pools);

  ~DescriptorPool();

  fml::StatusOr<vk::DescriptorSet> AllocateDescriptorSets(
      const vk::DescriptorSetLayout& layout,
      PipelineKey pipeline_key,
      const Context& context_vk);

 private:
  friend class DescriptorPoolRecycler;

  std::weak_ptr<const Context> context_;
  DescriptorCacheMap descriptor_sets_;
  std::vector<vk::UniqueDescriptorPool> pools_;

  void Destroy();

  fml::Status CreateNewPool(const Context& context_vk);

  DescriptorPool(const DescriptorPool&) = delete;

  DescriptorPool& operator=(const DescriptorPool&) = delete;
};

//------------------------------------------------------------------------------
/// @brief      Creates and manages the lifecycle of |vk::DescriptorPool|
///             objects.
class DescriptorPoolRecycler final
    : public std::enable_shared_from_this<DescriptorPoolRecycler> {
 public:
  ~DescriptorPoolRecycler() = default;

  /// The maximum number of descriptor pools this recycler will hold onto.
  static constexpr size_t kMaxRecycledPools = 32u;

  /// @brief      Creates a recycler for the given |Context|.
  ///
  /// @param[in]  context The context to create the recycler for.
  explicit DescriptorPoolRecycler(std::weak_ptr<Context> context)
      : context_(std::move(context)) {}

  /// @brief      Gets a descriptor pool.
  ///
  ///             This may create a new descriptor pool if no existing pools had
  ///             the necessary capacity.
  vk::UniqueDescriptorPool Get();

  std::shared_ptr<DescriptorPool> GetDescriptorPool();

  void Reclaim(DescriptorCacheMap descriptor_sets,
               std::vector<vk::UniqueDescriptorPool> pools);

 private:
  std::weak_ptr<Context> context_;

  Mutex recycled_mutex_;
  std::vector<std::shared_ptr<DescriptorPool>> recycled_
      IPLR_GUARDED_BY(recycled_mutex_);

  /// @brief      Creates a new |vk::CommandPool|.
  ///
  /// @returns    Returns a |std::nullopt| if a pool could not be created.
  vk::UniqueDescriptorPool Create();

  DescriptorPoolRecycler(const DescriptorPoolRecycler&) = delete;

  DescriptorPoolRecycler& operator=(const DescriptorPoolRecycler&) = delete;
};

}  // namespace ogre

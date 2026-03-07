// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "core/formats.h"
#include "core/sampler_descriptor.h"
#include "flutter/testing/testing.h"  // IWYU pragma: keep
#include "gtest/gtest.h"
#include "renderer/backend/vulkan/command_pool_vk.h"
#include "renderer/backend/vulkan/sampler_library_vk.h"
#include "renderer/backend/vulkan/test/mock_vulkan.h"
#include "renderer/backend/vulkan/workarounds_vk.h"

namespace ogre {
namespace testing {

TEST(SamplerLibraryVK, WorkaroundsCanDisableReadingFromMipLevels) {
  auto const context = MockVulkanContextBuilder().Build();

  auto library_vk =
      std::make_shared<SamplerLibraryVK>(context->GetDeviceHolder());
  std::shared_ptr<SamplerLibrary> library = library_vk;

  SamplerDescriptor desc;
  desc.mip_filter = MipFilter::kLinear;

  auto sampler = library->GetSampler(desc);
  EXPECT_EQ(sampler->GetDescriptor().mip_filter, MipFilter::kLinear);

  // Apply mips disabled workaround.
  library_vk->ApplyWorkarounds(WorkaroundsVK{.broken_mipmap_generation = true});

  sampler = library->GetSampler(desc);
  EXPECT_EQ(sampler->GetDescriptor().mip_filter, MipFilter::kBase);
}

}  // namespace testing
}  // namespace ogre

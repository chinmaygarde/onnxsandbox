#include "fixtures_location.h"
#include "gtest/gtest.h"

#include <onnxruntime/onnxruntime_cxx_api.h>

#include <absl/log/log.h>
#include <algorithm>
#include <string>
#include <vector>
#include "renderer/backend/vulkan/context_vk.h"

namespace ep::testing {

TEST(ExecutionProviderTest, MustFindCPUExecutionProvider) {
  std::vector<std::string> providers = Ort::GetAvailableProviders();
  ASSERT_FALSE(providers.empty());
  auto found =
      std::find(providers.begin(), providers.end(), "CPUExecutionProvider");
  EXPECT_NE(found, providers.end());
}

TEST(ExecutionProviderTest, CanCreateVulkanContext) {
  auto vulkan_dylib = fml::NativeLibrary::CreateForCurrentProcess();
  auto instance_proc_addr =
      vulkan_dylib->ResolveFunction<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  ASSERT_TRUE(instance_proc_addr.has_value());
  impeller::ContextVK::Settings settings;
  settings.shader_libraries_data = {};
  settings.proc_address_callback = instance_proc_addr.value();
  auto context = impeller::ContextVK::Create(std::move(settings));
  ASSERT_TRUE(!!context);
}

}  // namespace ep::testing

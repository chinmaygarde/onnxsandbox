#include "fixtures_location.h"
#include "gtest/gtest.h"

#include <onnxruntime/onnxruntime_cxx_api.h>

#include <absl/log/log.h>
#include <algorithm>
#include <string>
#include <vector>
#include "fml/build_config.h"
#include "fml/native_library.h"
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
#if defined(FML_OS_LINUX)
  auto vulkan_dylib = fml::NativeLibrary::Create("libvulkan.so.1");
#else
  auto vulkan_dylib = fml::NativeLibrary::CreateForCurrentProcess();
#endif
  auto instance_proc_addr =
      vulkan_dylib->ResolveFunction<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  ASSERT_TRUE(instance_proc_addr.has_value());
  ogre::Context::Settings settings;
  settings.shader_libraries_data = {};
  settings.proc_address_callback = instance_proc_addr.value();
  auto context = ogre::Context::Create(std::move(settings));
  if (!context) {
    GTEST_SKIP() << "Vulkan context could not be created (no compatible ICD).";
  }
}

}  // namespace ep::testing

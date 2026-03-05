#include "fixtures_location.h"
#include "gtest/gtest.h"

#include <onnxruntime/onnxruntime_cxx_api.h>

#include <absl/log/log.h>
#include <algorithm>
#include <string>
#include <vector>

namespace ep::testing {

TEST(HelloOnnx, CreateEnvironment) {
  Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "hello_onnx");
  SUCCEED();
}

TEST(HelloOnnx, MustFindCPUExecutionProvider) {
  std::vector<std::string> providers = Ort::GetAvailableProviders();
  ASSERT_FALSE(providers.empty());
  auto found =
      std::find(providers.begin(), providers.end(), "CPUExecutionProvider");
  EXPECT_NE(found, providers.end());
}

}  // namespace ep::testing

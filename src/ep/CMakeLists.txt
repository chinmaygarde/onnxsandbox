# Library
add_library(onnx_sandbox
  provider.cc
  provider.h
)

target_link_libraries(onnx_sandbox PUBLIC renderer)

# Configure Fixtures Location
get_filename_component(TEST_ASSETS_LOCATION ../assets ABSOLUTE)
configure_file(fixtures_location.h.in fixtures_location.h @ONLY)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(onnx_sandbox PUBLIC glm::glm)

# Abseil
find_package(absl CONFIG REQUIRED)
target_link_libraries(onnx_sandbox PUBLIC absl::log absl::base)

# ONNX Runtime
find_package(onnxruntime CONFIG REQUIRED)
target_link_libraries(onnx_sandbox PUBLIC onnxruntime::onnxruntime)

# Unittests
add_executable(onnx_sandbox_tests
  unittests.cc
  unittests_main.cc
  ${CMAKE_CURRENT_BINARY_DIR}/fixtures_location.h
)
target_link_libraries(onnx_sandbox_tests
  onnx_sandbox
  base_unittests
  core_unittests
  fml_unittests
  geometry_unittests
  GTest::gtest
  absl::log_initialize
  absl::log_globals
)
target_include_directories(onnx_sandbox_tests
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)
gtest_discover_tests(onnx_sandbox_tests)


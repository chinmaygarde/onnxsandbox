# Library
add_library(onnx_sandbox
  project.h
  project.cc
)

# Configure Fixtures Location
get_filename_component(TEST_ASSETS_LOCATION ../assets ABSOLUTE)
configure_file(fixtures_location.h.in fixtures_location.h @ONLY)

# Unittests
add_executable(onnx_sandbox_tests
  unittests.cc
  ${CMAKE_CURRENT_BINARY_DIR}/fixtures_location.h
)
target_link_libraries(onnx_sandbox_tests
  onnx_sandbox
  GTest::gtest_main
)
target_include_directories(onnx_sandbox_tests
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)
gtest_discover_tests(onnx_sandbox_tests)

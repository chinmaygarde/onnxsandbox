# Library
add_library(ogre
  provider.cc
  provider.h
)

target_link_libraries(ogre PUBLIC hal)

# Configure Fixtures Location
get_filename_component(TEST_ASSETS_LOCATION ../assets ABSOLUTE)
configure_file(fixtures_location.h.in fixtures_location.h @ONLY)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(ogre PUBLIC glm::glm)

# Abseil
find_package(absl CONFIG REQUIRED)
target_link_libraries(ogre PUBLIC absl::log absl::base)

# ONNX Runtime
find_package(onnxruntime CONFIG REQUIRED)
target_link_libraries(ogre PUBLIC onnxruntime::onnxruntime)

# Unittests
add_executable(ogre_tests
  unittests.cc
  unittests_main.cc
  ${CMAKE_CURRENT_BINARY_DIR}/fixtures_location.h
)
target_link_libraries(ogre_tests
  ogre
  base_unittests
  core_unittests
  fml_unittests
  geometry_unittests
  GTest::gtest
  absl::log_initialize
  absl::log_globals
)
target_include_directories(ogre_tests
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)
gtest_discover_tests(ogre_tests)


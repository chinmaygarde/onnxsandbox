add_library(core
  buffer_view.cc
  buffer_view.h
  device_buffer.cc
  device_buffer.h
  device_buffer_descriptor.cc
  device_buffer_descriptor.h
  formats.cc
  formats.h
  range.cc
  range.h
  raw_ptr.cc
  raw_ptr.h
  runtime_types.cc
  runtime_types.h
  sampler_descriptor.cc
  sampler_descriptor.h
  shader_types.cc
  shader_types.h
  texture.cc
  texture.h
  texture_descriptor.cc
  texture_descriptor.h
  vertex_buffer.cc
  vertex_buffer.h
)

target_link_libraries(core PUBLIC base geometry)

target_include_directories(core PUBLIC ..)

add_library(core_unittests OBJECT
  allocator_unittests.cc
  buffer_view_unittests.cc
)

target_link_libraries(core_unittests PUBLIC core GTest::gtest)

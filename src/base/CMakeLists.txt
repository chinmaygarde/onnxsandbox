add_library(base
  allocation.cc
  allocation.h
  allocation_size.cc
  allocation_size.h
  backend_cast.h
  comparable.cc
  comparable.h
  config.h
  flags.h
  mask.h
  promise.cc
  promise.h
  strings.cc
  strings.h
  thread.cc
  thread.h
  thread_safety.cc
  thread_safety.h
  timing.h
  validation.cc
  validation.h
  version.cc
  version.h
)

target_link_libraries(base PUBLIC fml)

add_library(base_unittests OBJECT
  allocation_size_unittests.cc
  base_unittests.cc
)

target_link_libraries(base_unittests PUBLIC base GTest::gtest)

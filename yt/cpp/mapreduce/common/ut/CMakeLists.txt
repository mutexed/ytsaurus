
# This file was generated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_executable(yt-cpp-mapreduce-common-ut)
target_include_directories(yt-cpp-mapreduce-common-ut PRIVATE
  ${CMAKE_SOURCE_DIR}/yt/cpp/mapreduce/common
)
target_link_libraries(yt-cpp-mapreduce-common-ut PUBLIC
  contrib-libs-linux-headers
  contrib-libs-cxxsupp
  yutil
  library-cpp-cpuid_check
  cpp-testing-unittest_main
  cpp-mapreduce-common
)
target_link_options(yt-cpp-mapreduce-common-ut PRIVATE
  -ldl
  -lrt
  -Wl,--no-as-needed
  -fPIC
  -fPIC
  -lpthread
  -lrt
  -ldl
)
target_sources(yt-cpp-mapreduce-common-ut PRIVATE
  ${CMAKE_SOURCE_DIR}/yt/cpp/mapreduce/common/helpers_ut.cpp
)
set_property(
  TARGET
  yt-cpp-mapreduce-common-ut
  PROPERTY
  SPLIT_FACTOR
  1
)
add_yunittest(
  NAME
  yt-cpp-mapreduce-common-ut
  TEST_TARGET
  yt-cpp-mapreduce-common-ut
  TEST_ARG
  --print-before-suite
  --print-before-test
  --fork-tests
  --print-times
  --show-fails
)
set_yunittest_property(
  TEST
  yt-cpp-mapreduce-common-ut
  PROPERTY
  LABELS
  SMALL
)
set_yunittest_property(
  TEST
  yt-cpp-mapreduce-common-ut
  PROPERTY
  PROCESSORS
  1
)
target_allocator(yt-cpp-mapreduce-common-ut
  cpp-malloc-tcmalloc
  libs-tcmalloc-no_percpu_cache
)
vcs_info(yt-cpp-mapreduce-common-ut)


# This file was generated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_executable(batch_request)
target_compile_options(batch_request PRIVATE
  -Wdeprecated-this-capture
)
target_link_libraries(batch_request PUBLIC
  contrib-libs-linux-headers
  contrib-libs-cxxsupp
  yutil
  library-cpp-cpuid_check
  cpp-mapreduce-client
)
target_link_options(batch_request PRIVATE
  -ldl
  -lrt
  -Wl,--no-as-needed
  -fPIC
  -fPIC
  -lpthread
  -lrt
  -ldl
  -lutil
)
target_sources(batch_request PRIVATE
  ${CMAKE_SOURCE_DIR}/yt/cpp/mapreduce/examples/tutorial/batch_request/main.cpp
)
target_allocator(batch_request
  cpp-malloc-tcmalloc
  libs-tcmalloc-no_percpu_cache
)
vcs_info(batch_request)

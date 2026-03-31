# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspace/host-app/third_party/src/googletest-src"
  "/workspace/host-app/third_party/src/googletest-build"
  "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix"
  "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/tmp"
  "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/src"
  "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspace/host-app/third_party/src/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

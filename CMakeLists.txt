cmake_minimum_required(VERSION 3.28)

project(func_wander)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*,modernize-*,-modernize-use-trailing-return-type,performance-*,portability-*,bugprone-*,-bugprone-exception-escape)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tests)
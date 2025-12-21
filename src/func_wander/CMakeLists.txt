cmake_minimum_required(VERSION 3.28)
project(func_wander VERSION 1.0.0 LANGUAGES CXX)

option(func_wander_BUILD_TESTS "Build tests for the library" OFF)
option(func_wander_INSTALL "Generate the install target" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(func_wander INTERFACE)
add_library(func_wander::func_wander ALIAS func_wander)

find_package(nlohmann_json REQUIRED)
target_link_libraries(func_wander INTERFACE nlohmann_json::nlohmann_json)

target_include_directories(func_wander INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(func_wander INTERFACE cxx_std_17)
set_target_properties(func_wander PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

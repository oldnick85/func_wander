# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.28)

# Library project inheriting version from parent
project(func_wander_lib VERSION ${func_wander_VERSION} LANGUAGES CXX)

# Find required dependencies
find_package(nlohmann_json 3.12.0 REQUIRED)

# Create interface library (header-only)
add_library(func_wander INTERFACE)
add_library(func_wander::func_wander ALIAS func_wander)

# Set target properties
set_target_properties(func_wander PROPERTIES
    INTERFACE_CXX_STANDARD 23
    INTERFACE_CXX_STANDARD_REQUIRED ON
    INTERFACE_CXX_EXTENSIONS OFF
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
    VERSION ${func_wander_VERSION}
    SOVERSION ${func_wander_VERSION_MAJOR}
)

# Link dependencies
target_link_libraries(func_wander INTERFACE 
    nlohmann_json::nlohmann_json
)

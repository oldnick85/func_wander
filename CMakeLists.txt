# Minimum required CMake version
cmake_minimum_required(VERSION 3.28)

# Project definition with metadata
project(func_wander
    VERSION 1.0.0
    DESCRIPTION "A brute-force approach to finding functions that satisfy given constraints"
    HOMEPAGE_URL "https://github.com/oldnick85/func_wander"
    LANGUAGES CXX
)

# C++ standard configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration options
option(func_wander_BUILD_TESTS "Build tests for the library" ON)
option(func_wander_BUILD_EXAMPLES "Build examples" ON)
option(func_wander_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(func_wander_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(func_wander_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(func_wander_ENABLE_COVERAGE "Enable code coverage" OFF)
option(func_wander_ENABLE_WERROR "Treat warnings as errors" ON)

# Automatically set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING 
        "Choose the type of build: Debug, Release, RelWithDebInfo, MinSizeRel" 
        FORCE)
endif()

# Convert build type to uppercase for comparison
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)

# Compiler flags based on build type
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Base compiler warning flags
add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)

# Disable warnings for external libraries
add_compile_options(
    -Wno-sign-conversion  # For nlohmann/json and other third-party libraries
    -Wno-conversion
)

# Treat warnings as errors if enabled
if(func_wander_ENABLE_WERROR)
    add_compile_options(-Werror)
endif()

# Clang-Tidy configuration for static analysis
if(func_wander_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXE}
            -checks=-*,readability-*,modernize-*,-modernize-use-trailing-return-type,
                    performance-*,portability-*,bugprone-*,-bugprone-exception-escape,
                    clang-analyzer-*,cppcoreguidelines-*
            --warnings-as-errors=*
        )
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "Clang-Tidy requested but executable not found")
    endif()
endif()

# AddressSanitizer configuration
if(func_wander_ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "AddressSanitizer enabled")
endif()

# UndefinedBehaviorSanitizer configuration
if(func_wander_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

# Code coverage configuration (for gcc/clang)
if(func_wander_ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    message(STATUS "Code coverage enabled")
endif()

# Compiler-specific optimizations and flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Additional warnings for GNU/Clang
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
            add_compile_options(-Wmisleading-indentation -Wduplicated-cond)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12)
            add_compile_options(-Wdocumentation -Wthread-safety)
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Windows/MSVC specific flags
    add_compile_options(/W4 /WX /permissive- /EHsc)
endif()

# Conan integration - automatically use toolchain if present
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    message(STATUS "Conan toolchain loaded")
else()
    message(STATUS "Building without Conan toolchain")
endif()

# Add subdirectories
add_subdirectory(src)

# Conditionally add examples directory
if(func_wander_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Conditionally add tests directory and enable testing
if(func_wander_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build tests: ${func_wander_BUILD_TESTS}")
message(STATUS "Build examples: ${func_wander_BUILD_EXAMPLES}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
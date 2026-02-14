# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.28)

# Early return if tests are disabled
if(NOT func_wander_BUILD_TESTS)
    return()
endif()

# Find testing framework
find_package(GTest 1.17.0 REQUIRED)
find_package(Threads REQUIRED)  # GTest may require threading

# Create test executable
add_executable(func_wander_tests
    main.cpp
)

# Test-specific preprocessor definitions
target_compile_definitions(func_wander_tests PRIVATE
    FUNC_WANDER_TESTING
)

# Link test executable with dependencies
target_link_libraries(func_wander_tests PRIVATE
    func_wander::func_wander
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Include directories for tests
target_include_directories(func_wander_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)

# Configure GoogleTest integration
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
    include(GoogleTest)
    gtest_discover_tests(func_wander_tests
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
else()
    # Fallback for older CMake versions
    add_test(NAME func_wander_tests COMMAND func_wander_tests)
endif()

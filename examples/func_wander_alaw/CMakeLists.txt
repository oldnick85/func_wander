# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.28)

# Early return if examples are disabled
if(NOT func_wander_BUILD_EXAMPLES)
    return()
endif()

# Find dependencies
#find_package(Curses REQUIRED)
find_package(argh REQUIRED)

# Example application
add_executable(func_wander_alaw
    main.cpp
)

# Link with dependencies
target_link_libraries(func_wander_alaw PRIVATE
    func_wander::func_wander
    argh
    )
    #${CURSES_LIBRARIES}



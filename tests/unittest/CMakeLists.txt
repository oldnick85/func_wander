find_package(GTest REQUIRED)

add_executable(unittest
    main.cpp
)

target_link_libraries(unittest
    PRIVATE
        GTest::GTest
)

target_link_libraries(unittest PRIVATE func_wander::func_wander)
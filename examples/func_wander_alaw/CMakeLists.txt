find_package(Curses REQUIRED)
find_package(argh REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(func_wander_alaw
    main.cpp
)

target_link_libraries(func_wander_alaw 
    PRIVATE 
        func_wander::func_wander 
        argh
        ${CURSES_LIBRARY}
)

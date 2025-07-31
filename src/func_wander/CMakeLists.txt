cmake_minimum_required(VERSION 3.28)
project(func_wander VERSION 1.0.0 LANGUAGES CXX)

# Опции проекта
option(func_wander_BUILD_TESTS "Build tests for the library" OFF)
option(func_wander_INSTALL "Generate the install target" ON)

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Определяем интерфейсную библиотеку (header-only)
add_library(func_wander INTERFACE)
add_library(func_wander::func_wander ALIAS func_wander)

find_package(nlohmann_json REQUIRED)
target_link_libraries(func_wander INTERFACE nlohmann_json::nlohmann_json)

# Добавляем include директории
target_include_directories(func_wander INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Опционально: устанавливаем свойства для IDE
target_compile_features(func_wander INTERFACE cxx_std_17)
set_target_properties(func_wander PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

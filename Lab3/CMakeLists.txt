cmake_minimum_required(VERSION 3.10)

project(Lab3Rabin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)

add_executable(rabin
    src/main.cpp
    src/rabin.cpp
    src/file_utils.cpp
)

target_include_directories(rabin PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(rabin PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

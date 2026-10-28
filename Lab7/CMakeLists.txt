cmake_minimum_required(VERSION 3.20)
project(EcElGamal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(elgamal_core STATIC
    src/bigint.cpp
    src/ec.cpp
    src/random.cpp
    src/elgamal.cpp
)
target_include_directories(elgamal_core PUBLIC src)

add_executable(elgamal src/main.cpp)
target_link_libraries(elgamal PRIVATE elgamal_core)

cmake_minimum_required(VERSION 3.20)
project(GostSignature LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(gost_core STATIC
    src/bigint.cpp
    src/ec.cpp
    src/random.cpp
    src/streebog.cpp
    src/gost3410.cpp
)
target_include_directories(gost_core PUBLIC src)

add_executable(gost3410 src/main.cpp)
target_link_libraries(gost3410 PRIVATE gost_core)

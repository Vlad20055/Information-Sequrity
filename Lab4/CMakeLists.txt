cmake_minimum_required(VERSION 3.20)
project(McEliece LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(mceliece_core STATIC
    src/gf2m.cpp
    src/random.cpp
    src/poly.cpp
    src/binmatrix.cpp
    src/goppa.cpp
    src/mceliece.cpp
)
target_include_directories(mceliece_core PUBLIC src)

add_executable(mceliece src/main.cpp)
target_link_libraries(mceliece PRIVATE mceliece_core)

cmake_minimum_required(VERSION 3.16)

project(cfp-usage LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libcfp REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example libcfp::libcfp)

cmake_minimum_required(VERSION 3.16)

project(mcfp-usage LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libmcfp REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example libmcfp::libmcfp)

cmake_minimum_required(VERSION 3.28)

project(mcfp-usage LANGUAGES CXX)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT (TARGET mcfp))
	find_package(mcfp REQUIRED)
endif()

add_executable(example example.cpp)
target_link_libraries(example mcfp::mcfp)

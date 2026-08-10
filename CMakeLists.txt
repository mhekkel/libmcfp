# Copyright (c) 2022-2025 Maarten L. Hekkelman
#
# SPDX-License-Identifier: BSD-2-Clause

# This makefile builds a traditional library, and when the compiler
# is thought to be good enough, a c++ module library as well.
# Both are installed. The users selects either the traditional
# library by using mcfp::mcfp as target library, or the module
# by using mcfp::mcfp-module

cmake_minimum_required(VERSION 3.28)

# Experimental import std support
if(${CMAKE_VERSION} VERSION_EQUAL 4.3.3)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

    set(CMAKE_CXX_STANDARD_REQUIRED OFF)
    set(CMAKE_CXX_MODULE_STD 1)
endif()

# set the project name
project(mcfp VERSION 2.0.4 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CPM)
include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(BUILD_DOCUMENTATION "Build the documentation" OFF)
option(MCFP_BUILD_CXX_MODULE "Build the library as a C++ 20 module library" ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if(MCFP_BUILD_CXX_MODULE)
        message(WARNING "Sorry, macOS has no support for module libraries, as fat as I can tell")
        set(MCFP_BUILD_CXX_MODULE OFF)
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(MCFP_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(MCFP_BUILD_CXX_MODULE OFF)
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	add_definitions(-DNOMINMAX)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    if(MCFP_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(MCFP_BUILD_CXX_MODULE OFF)
    endif()
endif()

try_compile(STD_CHARCONV_COMPILING
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
    message(NOTICE "mcfp: Using fast_float for std::from_chars")
    CPMFindPackage(
        NAME FastFloat
        GIT_REPOSITORY "https://github.com/fastfloat/fast_float"
        GIT_TAG v8.0.2
        EXCLUDE_FROM_ALL YES)
endif()

file(GLOB_RECURSE mcfp_headers include/*.hpp)
file(GLOB_RECURSE mcfp_modules include/*.cppm)
file(GLOB_RECURSE mcfp_cpp src/*.cpp)
file(GLOB_RECURSE mcfp_cxx src/*.cxx)

if(STD_CHARCONV_COMPILING)
    list(FILTER mcfp_cpp EXCLUDE REGEX src/charconv-fast_float\.cpp)
endif()

add_library(mcfp ${mcfp_cpp})
add_library(mcfp::mcfp ALIAS mcfp)

target_compile_features(mcfp PUBLIC cxx_std_23)
target_compile_definitions(mcfp PUBLIC USE_MODULE_STD=${CMAKE_CXX_MODULE_STD})

if(NOT STD_CHARCONV_COMPILING)
    target_include_directories(mcfp PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
	target_compile_definitions(mcfp PRIVATE USE_FAST_FLOAT)
endif()

target_sources(mcfp
    PUBLIC
    FILE_SET mcfp_classic TYPE HEADERS
    BASE_DIRS include
    FILES
    ${mcfp_headers})

set(mcfp_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(mcfp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_mcfp_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    COMPATIBLE_INTERFACE_STRING mcfp_MAJOR_VERSION)

install(TARGETS mcfp
    EXPORT mcfp-targets
    FILE_SET mcfp_classic
    DESTINATION "include")

install(EXPORT mcfp-targets
    NAMESPACE mcfp::
    DESTINATION lib/cmake/mcfp)

if(MCFP_BUILD_CXX_MODULE)
    add_library(mcfp-module ${mcfp_cxx})
    add_library(mcfp::mcfp-module ALIAS mcfp-module)

    target_compile_features(mcfp-module PUBLIC cxx_std_23)

    if(NOT STD_CHARCONV_COMPILING)
        target_include_directories(mcfp-module PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()

    target_sources(mcfp-module
        PUBLIC
        FILE_SET mcfp_module TYPE CXX_MODULES
        BASE_DIRS include
        FILES
        ${mcfp_modules})

    target_compile_definitions(mcfp-module PUBLIC MCFP_MODULE_MODE)

    target_include_directories(mcfp-module PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mcfp>)

    install(TARGETS mcfp-module
        EXPORT mcfp-module-targets
        CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
        FILE_SET mcfp_module DESTINATION "lib/cxx/miu")

    install(EXPORT mcfp-module-targets
        NAMESPACE mcfp::
        DESTINATION lib/cmake/mcfp
        FILE "mcfp-module-targets.cmake")

endif()

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/mcfp-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mcfp-config.cmake
    INSTALL_DESTINATION lib/cmake/mcfp)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mcfp-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mcfp-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mcfp-config-version.cmake"
    DESTINATION lib/cmake/mcfp)

set(generator -G "${CMAKE_GENERATOR}")

if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND generator -T "${CMAKE_GENERATOR_TOOLSET}")
endif()

if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND generator -A "${CMAKE_GENERATOR_PLATFORM}")
endif()

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
    add_subdirectory(examples)
endif()

if(BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif()

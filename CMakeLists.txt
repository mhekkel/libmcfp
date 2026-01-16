# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2022-2025 Maarten L. Hekkelman
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.28)

# set the project name
project(mcfp VERSION 2.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CTest)

if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
	set(CMAKE_CROSS_CONFIGS "Debug;Release")
	set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
elseif(NOT(CMAKE_GENERATOR STREQUAL "Ninja"))
	message(FATAL_ERROR "Ninja is required as generator")
endif()

option(BUILD_DOCUMENTATION "Build the documentation" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

try_compile(STD_CHARCONV_COMPILING
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
	message(NOTICE "mcfp: Using fast_float for std::from_chars")
	CPMFindPackage(
		NAME fast_float
		GIT_REPOSITORY "https://github.com/fastfloat/fast_float"
		GIT_TAG v8.0.2
		EXCLUDE_FROM_ALL YES)
endif()

file(GLOB_RECURSE mcfp_cppm modules/*.cppm)
file(GLOB_RECURSE mcfp_cpp src/*.cpp)

if(STD_CHARCONV_COMPILING)
	list(FILTER mcfp_cpp EXCLUDE REGEX src/charconv-fast_float\.cpp)
endif()

add_library(mcfp STATIC ${mcfp_cpp})
add_library(mcfp::mcfp ALIAS mcfp)

target_compile_features(mcfp PUBLIC cxx_std_20)

if(NOT STD_CHARCONV_COMPILING)
	target_include_directories(mcfp PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

if(MSVC)
	target_compile_definitions(mcfp INTERFACE NOMINMAX=1)
endif()

target_sources(mcfp
	PUBLIC
	FILE_SET mcfp_module TYPE CXX_MODULES
	FILES
	${mcfp_cppm})

# installation

set(mcfp_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(mcfp PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	INTERFACE_mcfp_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
	COMPATIBLE_INTERFACE_STRING mcfp_MAJOR_VERSION)

install(TARGETS mcfp
	EXPORT mcfp
	CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
	FILE_SET mcfp_module DESTINATION "lib/cxx/miu")

install(EXPORT mcfp
	NAMESPACE mcfp::
	DESTINATION lib/cmake/mcfp
	FILE "mcfp-targets.cmake")

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/mcfp-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mcfp-config.cmake
	INSTALL_DESTINATION lib/cmake/mcfp)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/mcfp-config-version.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcfp-config.cmake"
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
endif()

if(BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()

# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2022 Maarten L. Hekkelman
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
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

cmake_minimum_required(VERSION 3.16)

# set the project name
project(libmcfp VERSION 1.3.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DOCUMENTATION "Build the documentation" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	if("${CMAKE_CXX_COMPILER_VERSION}" LESS 9.4)
		message(FATAL_ERROR "Your GNU g++ is too old, need at least 9.4")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

if(MSVC)
	# make msvc standards compliant...
	add_compile_options(/permissive-)

    if(${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 10) # Windows 10
        add_definitions(-D _WIN32_WINNT=0x0A00)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.3) # Windows 8.1
        add_definitions(-D _WIN32_WINNT=0x0603)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.2) # Windows 8
        add_definitions(-D _WIN32_WINNT=0x0602)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.1) # Windows 7
        add_definitions(-D _WIN32_WINNT=0x0601)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.0) # Windows Vista
        add_definitions(-D _WIN32_WINNT=0x0600)
    else() # Windows XP (5.1)
        add_definitions(-D _WIN32_WINNT=0x0501)
    endif()

	add_definitions(-DNOMINMAX)
endif()

add_library(libmcfp INTERFACE)
add_library(libmcfp::libmcfp ALIAS libmcfp)

target_compile_features(libmcfp INTERFACE cxx_std_20)

# adding header sources just helps IDEs
target_sources(libmcfp PUBLIC
	FILE_SET mcfp_headers TYPE HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	include/mcfp/detail/charconv.hpp
	include/mcfp/detail/options.hpp
	include/mcfp/error.hpp
	include/mcfp/mcfp.hpp
	include/mcfp/text.hpp
	include/mcfp/utilities.hpp
)

# installation
install(TARGETS libmcfp
	EXPORT mcfp
	FILE_SET mcfp_headers
	DESTINATION include/)

install(EXPORT mcfp
	NAMESPACE libmcfp::
	DESTINATION lib/cmake/libmcfp
	FILE "libmcfpTargets.cmake")

configure_file(${PROJECT_SOURCE_DIR}/cmake/libmcfpConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/libmcfpConfig.cmake @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmcfpConfig.cmake"
	DESTINATION lib/cmake/libmcfp)

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

if(BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()


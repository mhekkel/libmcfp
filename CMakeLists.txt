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
project(libmcfp VERSION 1.2.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(Dart)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The minimum version of C++ required for this library")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTING "Build the unit test applications" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

if(MSVC)
	# make msvc standards compliant...
	add_compile_options(/permissive-)

	macro(get_WIN32_WINNT version)
		if(WIN32 AND CMAKE_SYSTEM_VERSION)
			set(ver ${CMAKE_SYSTEM_VERSION})
			string(REPLACE "." "" ver ${ver})
			string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

			set(${version} "0x${ver}")
		endif()
	endmacro()

	get_WIN32_WINNT(ver)
	add_definitions(-D_WIN32_WINNT=${ver})
endif()

add_library(libmcfp INTERFACE)
add_library(libmcfp::libmcfp ALIAS libmcfp)

target_include_directories(libmcfp INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# adding header sources just helps IDEs
target_sources(libmcfp INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:include>/mcfp/mcfp.hpp
)

set_target_properties(libmcfp PROPERTIES PUBLIC_HEADER include/mcfp/mcfp.hpp)

# installation
set(version_config "${CMAKE_CURRENT_BINARY_DIR}/libmcfpConfigVersion.cmake")

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

write_basic_package_version_file("${version_config}"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

install(TARGETS libmcfp
	EXPORT libmcfpConfig
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
	DIRECTORY include/mcfp
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Devel
)

export(TARGETS libmcfp NAMESPACE libmcfp:: FILE libmcfpTargets.cmake)

if(WIN32 AND NOT CYGWIN)
	set(CONFIG_LOC CMake)
else()
	set(CONFIG_LOC "${CMAKE_INSTALL_LIBDIR}/cmake/libmcfp")
endif()

configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/libmcfpConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/libmcfpConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmcfp
	PATH_VARS INCLUDE_INSTALL_DIR
)

install(EXPORT libmcfpConfig
	FILE libmcfpTargets.cmake
	NAMESPACE libmcfp::
	DESTINATION ${CONFIG_LOC})

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/libmcfpConfig.cmake "${version_config}"
	DESTINATION ${CONFIG_LOC})

if(ENABLE_TESTING)
	enable_testing()

	add_executable(libmcfp-unit-test ${PROJECT_SOURCE_DIR}/test/unit-test.cpp)

	target_link_libraries(libmcfp-unit-test libmcfp::libmcfp)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(libmcfp-unit-test PRIVATE /EHsc)
	endif()

	add_test(NAME libmcfp-unit-test
		COMMAND $<TARGET_FILE:libmcfp-unit-test> -- ${PROJECT_SOURCE_DIR}/test)
endif()

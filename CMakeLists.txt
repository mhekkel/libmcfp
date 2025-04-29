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

cmake_minimum_required(VERSION 3.23)

# set the project name
project(mcfp VERSION 1.4.2 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)

option(BUILD_DOCUMENTATION "Build the documentation" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	if("${CMAKE_CXX_COMPILER_VERSION}" LESS 9.4)
		message(FATAL_ERROR "Your GNU g++ is too old, need at least 9.4")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

add_library(mcfp INTERFACE)
add_library(mcfp::mcfp ALIAS mcfp)

target_compile_features(mcfp INTERFACE cxx_std_20)

# My god... why?
if(MSVC)
	target_compile_definitions(mcfp INTERFACE NOMINMAX=1)
endif()

# adding header sources just helps IDEs
target_sources(mcfp PUBLIC
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

# Clean up old config files (with old names)
file(GLOB OLD_CONFIG_FILES LIST_DIRECTORIES TRUE
	${CMAKE_INSTALL_FULL_LIBDIR}/cmake/mcfp/mcfp-*.cmake)

if(OLD_CONFIG_FILES)
	message(
		WARNING "Installation will remove old config files: ${OLD_CONFIG_FILES}")
	install(CODE "file(REMOVE_RECURSE ${OLD_CONFIG_FILES})")
endif()

# And an old include file
if(EXISTS ${CMAKE_INSTALL_FULL_INCLUDEDIR}/mcfp.hpp)
	message(
		WARNING "Installation will remove old header file: ${CMAKE_INSTALL_FULL_INCLUDEDIR}/mcfp.hpp")
	install(CODE "file(REMOVE ${CMAKE_INSTALL_FULL_INCLUDEDIR}/mcfp.hpp)")
endif()

install(TARGETS mcfp
	EXPORT mcfp
	FILE_SET mcfp_headers
	DESTINATION include/)

install(EXPORT mcfp
	NAMESPACE mcfp::
	DESTINATION lib/cmake/mcfp
	FILE "mcfpTargets.cmake")

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/mcfpConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mcfpConfig.cmake
	INSTALL_DESTINATION lib/cmake/mcfp)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/mcfpConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcfpConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/mcfpConfigVersion.cmake"
	DESTINATION lib/cmake/mcfp)

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

if(BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()

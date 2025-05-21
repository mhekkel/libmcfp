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

cmake_minimum_required(VERSION 3.31)

# set the project name
project(mcfp VERSION 2.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)

# option(BUILD_DOCUMENTATION "Build the documentation" OFF)

# if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
# elseif(MSVC)
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
# endif()

add_library(mcfp)
add_library(mcfp::mcfp ALIAS mcfp)

target_compile_features(mcfp PUBLIC cxx_std_20)

# # My god... why?
# if(MSVC)
# 	target_compile_definitions(mcfp PUBLIC NOMINMAX=1)
# endif()

# adding header sources just helps IDEs
target_sources(mcfp
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/sources/mcfp-utilities.cxx
	PUBLIC
	FILE_SET mcfp_headers TYPE CXX_MODULES
	FILES
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-charconv.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-config.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-error.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-options.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-text.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/modules/mcfp-utilities.ixx
)

# # installation

# install(TARGETS mcfp
# 	EXPORT mcfp
# 	FILE_SET mcfp_headers
# 	DESTINATION include/)

# install(EXPORT mcfp
# 	NAMESPACE mcfp::
# 	DESTINATION lib/cmake/mcfp
# 	FILE "mcfpTargets.cmake")

# configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/mcfpConfig.cmake.in
# 	${CMAKE_CURRENT_BINARY_DIR}/mcfpConfig.cmake
# 	INSTALL_DESTINATION lib/cmake/mcfp)
# write_basic_package_version_file(
# 	${CMAKE_CURRENT_BINARY_DIR}/mcfpConfigVersion.cmake
# 	VERSION ${PROJECT_VERSION}
# 	COMPATIBILITY AnyNewerVersion)

# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcfpConfig.cmake"
# 	"${CMAKE_CURRENT_BINARY_DIR}/mcfpConfigVersion.cmake"
# 	DESTINATION lib/cmake/mcfp)

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

if(BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()

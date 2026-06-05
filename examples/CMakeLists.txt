# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.28)

project(mcfp-usage LANGUAGES CXX)

if(NOT (TARGET mcfp))
	find_package(mcfp REQUIRED)
endif()

# Build using traditional headers
add_executable(example example.cpp)
target_link_libraries(example mcfp::mcfp)

if(MCFP_BUILD_CXX_MODULE)
	# Build using c++ module library
	add_executable(example-m example.cpp)
	target_link_libraries(example-m mcfp::mcfp-module)
endif()

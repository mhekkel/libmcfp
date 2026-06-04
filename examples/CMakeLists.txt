# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.28)

project(mcfp-usage LANGUAGES CXX)

if(NOT (TARGET mcfp))
	find_package(mcfp REQUIRED)
endif()

add_executable(example example.cpp)
target_link_libraries(example mcfp::mcfp)

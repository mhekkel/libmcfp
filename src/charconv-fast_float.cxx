// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#define MCFP_INCLUDE_HEADERS
#include "charconv-fast_float.cpp"
#undef MCFP_INCLUDE_HEADERS

module mcfp;

#define MCFP_INCLUDE_CODE

#include "charconv-fast_float.cpp"

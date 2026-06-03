// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

// #include "fast_float/fast_float.h"

#if __has_include(<experimental/type_traits>)
# include <experimental/type_traits>
#endif
#include <charconv>
#include <type_traits>
#include <utility>

export module mcfp:charconv;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "charconv.hpp"

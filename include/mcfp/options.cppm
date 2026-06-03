// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if __has_include(<experimental/type_traits>)
# include <experimental/type_traits>
#endif
#include <cassert>
#include <charconv>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

export module mcfp:options;

import :charconv;
import :error;
import :text;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "options.hpp"
// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

/**
 * @file text.cppm
 * This file contains word wrapping code
 */

# include <algorithm>
# include <cctype>
# include <cstdint>
# include <limits>
# include <string_view>
# include <vector>

export module mcfp:text;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "text.hpp"

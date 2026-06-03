// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

export module mcfp:sections;

import :options;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "sections.hpp"


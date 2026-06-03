// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

/**
 * @file error.cppm
 *
 * Header file containing the error codes used by libmcfp
 *
 */

#include <string>
#include <system_error>
#include <type_traits>

export module mcfp:error;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "error.hpp"


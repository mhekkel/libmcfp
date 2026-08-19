// Copyright Maarten L. Hekkelman 2022-2026
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * @file error.hpp
 *
 * Header file containing the error codes used by libmcfp
 *
 */

#ifndef MCFP_MODULE_MODE
# include "mcfp/export.hpp"

# include <string>
# include <system_error>
# include <type_traits>
# include <utility>
#endif

namespace mcfp
{

// we use the new system_error stuff.

/**
 * @enum config_error error.hpp mcfp/error.hpp
 *
 * @brief A stronly typed class containing the error codes reported by @ref mcfp::config
 */
MCFP_EXPORT enum class MCFP_API config_error {
	unknown_option = 1,              /**< The option requested does not exist, was not part of @ref mcfp::config::init. This error is returned by @ref mcfp::config::get */
	missing_argument_for_option,     /**< A option without a required argument was found while parsing the command line arguments */
	option_not_specified,            /**< There was not option found on the command line and no default argument was specified for the option passed in @ref mcfp::config::get */
	invalid_config_file,             /**< The config file is not of the expected format */
	wrong_type_cast,                 /**< An attempt was made to ask for an option in another type than used when registering this option in @ref mcfp::config::init */
	wrong_type_cast_flag,            /**< The value assigned in a config file to a flag option was not 'true', 'false' or an integral numerical value */
	config_file_not_found            /**< The specified config file was not found */
};
/**
 * @brief The implementation for config_category error messages
 *
 */

/**
 * @brief Return the implementation for the config_category
 *
 * @return std::error_category&
 */
MCFP_EXPORT MCFP_API std::error_category &config_category();

/**
 * @brief Create an std::error_code for our config_error enum
 *
 * @param e A config_error enum
 * @return std::error_code
 */
MCFP_EXPORT MCFP_INLINE std::error_code make_error_code(config_error e)
{
	return { static_cast<int>(e), config_category() };
}

} // namespace mcfp

// Make our error_codes implicitly convertible
template <> // NOLINT(bugprone-std-namespace-modification,cert-dcl58-cpp)
struct std::is_error_code_enum<mcfp::config_error>
	: public std::true_type
{
};

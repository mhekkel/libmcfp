// Copyright Maarten L. Hekkelman 2022-2025
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
MCFP_EXPORT enum class config_error
{
	unknown_option = 1,              /**< The option requested does not exist, was not part of @ref mcfp::config::init. This error is returned by @ref mcfp::config::get */
	option_does_not_accept_argument, /**< When parsing the command line arguments a value (argument) was specified for an option that should not have one */
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
MCFP_EXPORT class config_category_impl : public std::error_category
{
  public:
	/**
	 * @brief User friendly name
	 *
	 * @return const char*
	 */

	[[nodiscard]] const char *name() const noexcept override
	{
		return "configuration";
	}

	/**
	 * @brief Provide the error message as a string for the error code @a ev
	 *
	 * @param ev The error code
	 * @return std::string
	 */

	[[nodiscard]] std::string message(int ev) const override
	{
		switch (static_cast<config_error>(ev))
		{
			case config_error::unknown_option:
				return "unknown option";
			case config_error::option_does_not_accept_argument:
				return "option does not accept argument";
			case config_error::missing_argument_for_option:
				return "missing argument for option";
			case config_error::option_not_specified:
				return "option was not specified";
			case config_error::invalid_config_file:
				return "config file contains a syntax error";
			case config_error::wrong_type_cast:
				return "the implementation contains a type cast error";
			case config_error::config_file_not_found:
				return "the specified config file was not found";
			case config_error::wrong_type_cast_flag:
				return "the value assigned in a config file to a flag option was not 'true', 'false' or an integral numerical value";
		}
		std::unreachable();
	}

	/**
	 * @brief Return whether two error codes are equivalent, always false in this case
	 *
	 */

	[[nodiscard]] bool equivalent(const std::error_code & /*code*/, int /*condition*/) const noexcept override
	{
		return false;
	}
};

/**
 * @brief Return the implementation for the config_category
 *
 * @return std::error_category&
 */
MCFP_EXPORT MCFP_INLINE std::error_category &config_category()
{
	static config_category_impl instance;
	return instance;
}

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

/**
 * @brief Create an std::error_condition for our config_error enum
 *
 * @param e A config_error enum
 * @return std::error_condition
 */
MCFP_EXPORT MCFP_INLINE std::error_condition make_error_condition(config_error e)
{
	return { static_cast<int>(e), config_category() };
}

} // namespace mcfp

// Make our error_codes implicitly convertible

namespace std
{

template <> // NOLINT(bugprone-std-namespace-modification,cert-dcl58-cpp)
struct is_error_condition_enum<mcfp::config_error>
    : public true_type
{
};

} // namespace std

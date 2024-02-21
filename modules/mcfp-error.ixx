/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

module;

#include <cassert>
#include <string>
#include <system_error>

export module mcfp:error;

/**
 * @file error.hpp
 *
 * Header file containing the error codes used by libmcfp
 *
 */

namespace mcfp
{

// we use the new system_error stuff.

/**
 * @enum config_error error.hpp mcfp/error.hpp
 * 
 * @brief A stronly typed class containing the error codes reported by @ref mcfp::config
 */
export enum class config_error
{
	unknown_option = 1,              /**< The option requested does not exist, was not part of @ref mcfp::config::init. This error is returned by @ref mcfp::config::get */
	option_does_not_accept_argument, /**< When parsing the command line arguments a value (argument) was specified for an option that should not have one */
	missing_argument_for_option,     /**< A option without a required argument was found while parsing the command line arguments */
	option_not_specified,            /**< There was not option found on the command line and no default argument was specified for the option passed in @ref mcfp::config::get */
	invalid_config_file,             /**< The config file is not of the expected format */
	wrong_type_cast,                 /**< An attempt was made to ask for an option in another type than used when registering this option in @ref mcfp::config::init */
	config_file_not_found            /**< The specified config file was not found */
};
/**
 * @brief The implementation for @ref config_category error messages
 *
 */
export class config_category_impl : public std::error_category
{
  public:
	/**
	 * @brief User friendly name
	 *
	 * @return const char*
	 */

	const char *name() const noexcept override
	{
		return "configuration";
	}

	/**
	 * @brief Provide the error message as a string for the error code @a ev
	 *
	 * @param ev The error code
	 * @return std::string
	 */

	std::string message(int ev) const override
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
			default:
				assert(false);
				return "unknown error code";
		}
	}

	/**
	 * @brief Return whether two error codes are equivalent, always false in this case
	 *
	 */

	bool equivalent(const std::error_code & /*code*/, int /*condition*/) const noexcept override
	{
		return false;
	}
};

/**
 * @brief Return the implementation for the config_category
 *
 * @return std::error_category&
 */
export std::error_category &config_category()
{
	static config_category_impl instance;
	return instance;
}

export std::error_code make_error_code(config_error e)
{
	return std::error_code(static_cast<int>(e), config_category());
}

export std::error_condition make_error_condition(config_error e)
{
	return std::error_condition(static_cast<int>(e), config_category());
}

} // namespace mcfp
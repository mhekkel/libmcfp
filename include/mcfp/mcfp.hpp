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

#pragma once

/// \file
/// This header-only library contains code to parse argc/argv and store the
/// values provided into a singleton object.

#include <cassert>
#include <cstring>

#include <any>
#include <charconv>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include <mcfp/error.hpp>
#include <mcfp/text.hpp>
#include <mcfp/utilities.hpp>
#include <mcfp/detail/options.hpp>

namespace mcfp
{

// --------------------------------------------------------------------
/**
 * @brief A singleton class. Use @ref mcfp::config::instance to create and/or
 * retrieve the single instance
 * 
 */

class config
{
	using option_base = detail::option_base;

  public:

	/**
	 * @brief Set the 'usage' string
	 * 
	 * @param usage The usage message
	 */
	void set_usage(std::string_view usage)
	{
		m_usage = usage;
	}

	/**
	 * @brief Initialise a config instance with a \a usage message and a set of \a options
	 * 
	 * @param usage The usage message
	 * @param options Variadic list of options recognised by this config object, use @ref mcfp::make_option and variants to create these
	 */
	template <typename... Options>
	void init(std::string_view usage, Options... options)
	{
		m_usage = usage;
		m_impl.reset(new config_impl(std::forward<Options>(options)...));
	}

	/**
	 * @brief Set the ignore unknown flag
	 * 
	 * @param ignore_unknown When true, unknown options are simply ignored instead of
	 * throwing an error
	 */
	void set_ignore_unknown(bool ignore_unknown)
	{
		m_ignore_unknown = ignore_unknown;
	}

	/**
	 * @brief Use this to retrieve the single instance of this class
	 * 
	 * @return config& The singleton instance
	 */
	static config &instance()
	{
		static std::unique_ptr<config> s_instance;
		if (not s_instance)
			s_instance.reset(new config);
		return *s_instance;
	}

	/**
	 * @brief Simply return true if the option with \a name has a value assigned
	 * 
	 * @param name The name of the option
	 * @return bool Returns true when the option has a value
	 */
	bool has(std::string_view name) const
	{
		auto opt = m_impl->get_option(name);
		return opt != nullptr and (opt->m_seen > 0 or opt->m_has_default);
	}

	/**
	 * @brief Return how often an option with the name \a name was seen.
	 * Use e.g. to increase verbosity level
	 * 
	 * @param name The name of the option to check
	 * @return int The count for the named option
	 */
	int count(std::string_view name) const
	{
		auto opt = m_impl->get_option(name);
		return opt ? opt->m_seen : 0;
	}

	/**
	 * @brief Returns the value for the option with name \a name. Throws
	 * an exception if the option has not value assigned
	 * 
	 * @tparam T The type of the value requested.
	 * @param name The name of the option requested
	 * @return auto The value of the named option
	 */
	template <typename T>
	auto get(std::string_view name) const
	{
		using return_type = std::remove_cv_t<T>;

		std::error_code ec;
		return_type result = get<T>(name, ec);

		if (ec)
			throw std::system_error(ec, std::string{ name });

		return result;
	}

	/**
	 * @brief Returns the value for the option with name \a name. If
	 * the option has no value assigned or is of a wrong type,
	 * ec is set to an appropriate error
	 * 
	 * @tparam T The type of the value requested.
	 * @param name The name of the option requested
	 * @param ec The error status is returned in this variable
	 * @return auto The value of the named option
	 */
	template <typename T>
	auto get(std::string_view name, std::error_code &ec) const
	{
		using return_type = std::remove_cv_t<T>;

		return_type result{};
		auto opt = m_impl->get_option(name);

		if (opt == nullptr)
			ec = make_error_code(config_error::unknown_option);
		else
		{
			std::any value = opt->get_value();

			if (not value.has_value())
				ec = make_error_code(config_error::option_not_specified);
			else
			{
				try
				{
					result = std::any_cast<T>(value);
				}
				catch (const std::bad_cast &)
				{
					ec = make_error_code(config_error::wrong_type_cast);
				}
			}
		}

		return result;
	}

	/**
	 * @brief Return the std::string value of the option with name \a name
	 * If no value was assigned, or the type of the option cannot be casted
	 * to a string, an exception is thrown.
	 * 
	 * @param name The name of the option value requested
	 * @return std::string The value of the option
	 */
	std::string get(std::string_view name) const
	{
		return get<std::string>(name);
	}

	/**
	 * @brief Return the std::string value of the option with name \a name
	 * If no value was assigned, or the type of the option cannot be casted
	 * to a string, an error is returned in \a ec.
	 * 
	 * @param name The name of the option value requested
	 * @param ec The error status is returned in this variable
	 * @return std::string The value of the option
	 */
	std::string get(std::string_view name, std::error_code &ec) const
	{
		return get<std::string>(name, ec);
	}

	/**
	 * @brief Return the list of operands.
	 * 
	 * @return const std::vector<std::string>& The operand as a vector of strings
	 */
	const std::vector<std::string> &operands() const
	{
		return m_impl->m_operands;
	}

	/**
	 * @brief Write the configuration to the std::ostream \a os
	 * This will print the usage string and each of the configured
	 * options along with their optional default value as well as
	 * their help string
	 * 
	 * @param os The std::ostream to write to, usually std::cout or std::cerr
	 * @param conf The config object to write out
	 * @return std::ostream& Returns the parameter \a os
	 */
	friend std::ostream &operator<<(std::ostream &os, const config &conf)
	{
		size_t terminal_width = get_terminal_width();

		if (not conf.m_usage.empty())
			os << conf.m_usage << std::endl;

		size_t options_width = conf.m_impl->get_option_width();

		if (options_width > terminal_width / 2)
			options_width = terminal_width / 2;

		conf.m_impl->write(os, options_width);

		return os;
	}

	// --------------------------------------------------------------------

	/**
	 * @brief Parse the \a argv vector containing \a argc elements. Throws
	 * an exception if any error was found
	 * 
	 * @param argc The number of elements in \a argv
	 * @param argv The vector of command line arguments
	 */
	void parse(int argc, const char *const argv[])
	{
		std::error_code ec;
		parse(argc, argv, ec);
		if (ec)
			throw std::system_error(ec);
	}

	/**
	 * @brief Parse a configuration file called \a config_file_name optionally
	 * specified on the command line with option \a config_option
	 * The file is searched for in each of the directories specified in \a search_dirs
	 * This function throws an exception if an error was found during processing
	 * 
	 * @param config_option The name of the option used to specify the config file
	 * @param config_file_name The default name of the option file to use if the config
	 * option was not specified on the command line
	 * @param search_dirs The list of directories to search for the config file
	 */
	void parse_config_file(std::string_view config_option, std::string_view config_file_name,
		std::initializer_list<std::string_view> search_dirs)
	{
		std::error_code ec;
		parse_config_file(config_option, config_file_name, search_dirs, ec);
		if (ec)
			throw std::system_error(ec);
	}

	/**
	 * @brief Parse a configuration file called \a config_file_name optionally
	 * specified on the command line with option \a config_option
	 * The file is searched for in each of the directories specified in \a search_dirs
	 * If an error is found it is returned in the variable \a ec
	 * 
	 * @param config_option The name of the option used to specify the config file
	 * @param config_file_name The default name of the option file to use if the config
	 * option was not specified on the command line
	 * @param search_dirs The list of directories to search for the config file
	 * @param ec The variable containing the error status
	 */
	void parse_config_file(std::string_view config_option, std::string_view config_file_name,
		std::initializer_list<std::string_view> search_dirs, std::error_code &ec)
	{
		std::string file_name{ config_file_name };
		bool parsed_config_file = false;

		if (has(config_option))
			file_name = get<std::string>(config_option);

		for (std::filesystem::path dir : search_dirs)
		{
			std::ifstream file(dir / file_name);

			if (not file.is_open())
				continue;

			parse_config_file(file, ec);
			parsed_config_file = true;
			break;
		}

		if (not parsed_config_file and has(config_option))
			ec = make_error_code(config_error::config_file_not_found);
	}

	/**
	 * @brief Parse a configuration file specified by \a file
	 * If an error is found it is returned in the variable \a ec
	 * 
	 * @param file The path to the config file
	 * @param ec The variable containing the error status
	 */
	void parse_config_file(const std::filesystem::path &file, std::error_code &ec)
	{
		std::ifstream is(file);
		if (is.is_open())
			parse_config_file(is, ec);
	}

  private:

	static bool is_name_char(int ch)
	{
		return std::isalnum(ch) or ch == '_' or ch == '-';
	}

	static constexpr bool is_eoln(int ch)
	{
		return ch == '\n' or ch == '\r' or ch == std::char_traits<char>::eof();
	}

  public:

	/**
	 * @brief Parse the configuration file in \a is
	 * If an error is found it is returned in the variable \a ec
	 * 
	 * @param is A std::istream for the contents of a config file
	 * @param ec The variable containing the error status
	 */
	void parse_config_file(std::istream &is, std::error_code &ec)
	{
		auto &buffer = *is.rdbuf();

		enum class State
		{
			NAME_START,
			COMMENT,
			NAME,
			ASSIGN,
			VALUE_START,
			VALUE
		} state = State::NAME_START;

		std::string name, value;

		for (;;)
		{
			auto ch = buffer.sbumpc();

			switch (state)
			{
				case State::NAME_START:
					if (is_name_char(ch))
					{
						name = { static_cast<char>(ch) };
						value.clear();
						state = State::NAME;
					}
					else if (ch == '#')
						state = State::COMMENT;
					else if (ch != ' ' and ch != '\t' and not is_eoln(ch))
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::COMMENT:
					if (is_eoln(ch))
						state = State::NAME_START;
					break;

				case State::NAME:
					if (is_name_char(ch))
						name.insert(name.end(), static_cast<char>(ch));
					else if (is_eoln(ch))
					{
						auto opt = m_impl->get_option(name);

						if (opt == nullptr)
						{
							if (not m_ignore_unknown)
								ec = make_error_code(config_error::unknown_option);
						}
						else if (not opt->m_is_flag)
							ec = make_error_code(config_error::missing_argument_for_option);
						else
							++opt->m_seen;

						state = State::NAME_START;
					}
					else
					{
						buffer.sungetc();
						state = State::ASSIGN;
					}
					break;

				case State::ASSIGN:
					if (ch == '=')
						state = State::VALUE_START;
					else if (is_eoln(ch))
					{
						auto opt = m_impl->get_option(name);

						if (opt == nullptr)
						{
							if (not m_ignore_unknown)
								ec = make_error_code(config_error::unknown_option);
						}
						else if (not opt->m_is_flag)
							ec = make_error_code(config_error::missing_argument_for_option);
						else
							++opt->m_seen;

						state = State::NAME_START;
					}
					else if (ch != ' ' and ch != '\t')
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::VALUE_START:
				case State::VALUE:
					if (is_eoln(ch))
					{
						auto opt = m_impl->get_option(name);

						if (opt == nullptr)
						{
							if (not m_ignore_unknown)
								ec = make_error_code(config_error::unknown_option);
						}
						else if (opt->m_is_flag)
							ec = make_error_code(config_error::option_does_not_accept_argument);
						else if (not value.empty() and (opt->m_seen == 0 or opt->m_multi))
						{
							opt->set_value(value, ec);
							++opt->m_seen;
						}

						state = State::NAME_START;
					}
					else if (state == State::VALUE)
						value.insert(value.end(), static_cast<char>(ch));
					else if (ch != ' ' and ch != '\t')
					{
						value = { static_cast<char>(ch) };
						state = State::VALUE;
					}
					break;
			}

			if (ec or ch == std::char_traits<char>::eof())
				break;
		}
	}

	/**
	 * @brief Parse the \a argv vector containing \a argc elements.
	 * In case of an error, the error is returned in \a ec
	 * 
	 * @param argc The number of elements in \a argv
	 * @param argv The vector of command line arguments
	 * @param ec The variable receiving the error status
	 */
	void parse(int argc, const char *const argv[], std::error_code &ec)
	{
		using namespace std::literals;

		enum class State
		{
			options,
			operands
		} state = State::options;

		for (int i = 1; i < argc and not ec; ++i)
		{
			const char *arg = argv[i];

			if (arg == nullptr) // should not happen
				break;

			if (state == State::options)
			{
				if (*arg != '-') // according to POSIX this is the end of options, start operands
				                 // state = State::operands;
				{                // however, people nowadays expect to be able to mix operands and options
					m_impl->m_operands.emplace_back(arg);
					continue;
				}
				else if (arg[1] == '-' and arg[2] == 0)
				{
					state = State::operands;
					continue;
				}
			}

			if (state == State::operands)
			{
				m_impl->m_operands.emplace_back(arg);
				continue;
			}

			option_base *opt = nullptr;
			std::string_view opt_arg;

			assert(*arg == '-');
			++arg;

			if (*arg == '-') // double --, start of new argument
			{
				++arg;

				assert(*arg != 0); // this should not happen, as it was checked for before

				std::string_view s_arg(arg);
				std::string_view::size_type p = s_arg.find('=');

				if (p != std::string_view::npos)
				{
					opt_arg = s_arg.substr(p + 1);
					s_arg = s_arg.substr(0, p);
				}

				opt = m_impl->get_option(s_arg);
				if (opt == nullptr)
				{
					if (not m_ignore_unknown)
						ec = make_error_code(config_error::unknown_option);
					continue;
				}

				if (opt->m_is_flag)
				{
					if (not opt_arg.empty())
						ec = make_error_code(config_error::option_does_not_accept_argument);

					++opt->m_seen;
					continue;
				}

				++opt->m_seen;
			}
			else // single character options
			{
				bool expect_option_argument = false;

				while (*arg != 0 and not ec)
				{
					opt = m_impl->get_option(*arg++);

					if (opt == nullptr)
					{
						if (not m_ignore_unknown)
							ec = make_error_code(config_error::unknown_option);
						continue;
					}

					++opt->m_seen;
					if (opt->m_is_flag)
						continue;

					opt_arg = arg;
					expect_option_argument = true;
					break;
				}

				if (not expect_option_argument)
					continue;
			}

			if (opt_arg.empty() and i + 1 < argc) // So, the = character was not present, the next arg must be the option argument
			{
				++i;
				opt_arg = argv[i];
			}

			if (opt_arg.empty())
				ec = make_error_code(config_error::missing_argument_for_option);
			else
				opt->set_value(opt_arg, ec);
		}
	}

  private:
	config() = default;
	config(const config &) = delete;
	config &operator=(const config &) = delete;

	struct config_impl_base
	{
		virtual ~config_impl_base() = default;

		virtual option_base *get_option(std::string_view name) = 0;
		virtual option_base *get_option(char short_name) = 0;

		virtual size_t get_option_width() const = 0;
		virtual void write(std::ostream &os, size_t width) const = 0;

		std::vector<std::string> m_operands;
	};

	template <typename... Options>
	struct config_impl : public config_impl_base
	{
		static constexpr size_t N = sizeof...(Options);

		config_impl(Options... options)
			: m_options(std::forward<Options>(options)...)
		{
		}

		option_base *get_option(std::string_view name) override
		{
			return get_option<0>(name);
		}

		template <size_t Ix>
		option_base *get_option([[maybe_unused]] std::string_view name)
		{
			if constexpr (Ix == N)
				return nullptr;
			else
			{
				option_base &opt = std::get<Ix>(m_options);
				return (opt.m_name == name) ? &opt : get_option<Ix + 1>(name);
			}
		}

		option_base *get_option(char short_name) override
		{
			return get_option<0>(short_name);
		}

		template <size_t Ix>
		option_base *get_option([[maybe_unused]] char short_name)
		{
			if constexpr (Ix == N)
				return nullptr;
			else
			{
				option_base &opt = std::get<Ix>(m_options);
				return (opt.m_short_name == short_name) ? &opt : get_option<Ix + 1>(short_name);
			}
		}

		virtual size_t get_option_width() const override
		{
			return std::apply([](Options const& ...opts) {
				size_t width = 0;
				((width = std::max(width, opts.width())), ...);
				return width;
			}, m_options);
		}

		virtual void write(std::ostream &os, size_t width) const override
		{
			std::apply([&os,width](Options const& ...opts) {
				(opts.write(os, width), ...);
			}, m_options);
		}

		std::tuple<Options...> m_options;
	};

	std::unique_ptr<config_impl_base> m_impl;
	bool m_ignore_unknown = false;
	std::string m_usage;
};

// --------------------------------------------------------------------

/**
 * @brief Create an option with name \a name and without a default value.
 * If \a T is void the option does not expect a value and is in fact a flag. 
 * 
 * If the type of \a T is a container (std::vector e.g.) the option can be
 * specified multiple times on the command line. 
 * 
 * The name \a name may end with a comma and a single character. This last
 * character will then be the short version whereas the leading characters
 * make up the long version.
 * 
 * @tparam T The type of the option
 * @param name The name of the option
 * @param description The help text for this option
 * @return auto The option object created
 */
template <typename T = void, std::enable_if_t<not detail::is_container_type_v<T>, int> = 0>
auto make_option(std::string_view name, std::string_view description)
{
	return detail::option<T>(name, description, false);
}

template <typename T, std::enable_if_t<detail::is_container_type_v<T>, int> = 0>
auto make_option(std::string_view name, std::string_view description)
{
	return detail::multiple_option<T>(name, description, false);
}

/**
 * @brief Create an option with name \a name and with a default value \a v.
 * 
 * If the type of \a T is a container (std::vector e.g.) the option can be
 * specified multiple times on the command line. 
 * 
 * The name \a name may end with a comma and a single character. This last
 * character will then be the short version whereas the leading characters
 * make up the long version.
 * 
 * @tparam T The type of the option
 * @param name The name of the option
 * @param v The default value to use
 * @param description The help text for this option
 * @return auto The option object created
 */
template <typename T, std::enable_if_t<not detail::is_container_type_v<T>, int> = 0>
auto make_option(std::string_view name, const T &v, std::string_view description)
{
	return detail::option<T>(name, v, description, false);
}

/**
 * @brief Create an option with name \a name and without a default value.
 * If \a T is void the option does not expect a value and is in fact a flag. 
 * This option will not be shown in the help / usage output.
 * 
 * If the type of \a T is a container (std::vector e.g.) the option can be
 * specified multiple times on the command line. 
 * 
 * The name \a name may end with a comma and a single character. This last
 * character will then be the short version whereas the leading characters
 * make up the long version.
 * 
 * @tparam T The type of the option
 * @param name The name of the option
 * @param description The help text for this option
 * @return auto The option object created
 */
template <typename T = void, std::enable_if_t<not detail::is_container_type_v<T>, int> = 0>
auto make_hidden_option(std::string_view name, std::string_view description)
{
	return detail::option<T>(name, description, true);
}

template <typename T, std::enable_if_t<detail::is_container_type_v<T>, int> = 0>
auto make_hidden_option(std::string_view name, std::string_view description)
{
	return detail::multiple_option<T>(name, description, true);
}

/**
 * @brief Create an option with name \a name and with default value \a v.
 * If \a T is void the option does not expect a value and is in fact a flag. 
 * This option will not be shown in the help / usage output.
 * 
 * If the type of \a T is a container (std::vector e.g.) the option can be
 * specified multiple times on the command line. 
 * 
 * The name \a name may end with a comma and a single character. This last
 * character will then be the short version whereas the leading characters
 * make up the long version.
 * 
 * @tparam T The type of the option
 * @param name The name of the option
 * @param v The default value to use
 * @param description The help text for this option
 * @return auto The option object created
 */
template <typename T, std::enable_if_t<not detail::is_container_type_v<T>, int> = 0>
auto make_hidden_option(std::string_view name, const T &v, std::string_view description)
{
	return detail::option<T>(name, v, description, true);
}

} // namespace mcfp

namespace std
{

template <>
struct is_error_condition_enum<mcfp::config_error>
	: public true_type
{
};

} // namespace std
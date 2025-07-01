/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2025 Maarten L. hekkelman
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

#include "mcfp/error.hpp"
#include "mcfp/text.hpp"
#include "mcfp/utilities.hpp"

#include "mcfp/detail/sections.hpp"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include <cassert>
#include <cstring>

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
	 * in a so-called global section (no leading section name for the options when using get or has)
	 * 
	 * This method also initialises all predefined (library) sections.
	 *
	 * @param usage The usage message
	 * @param options Variadic list of options recognised by this config object, use mcfp::make_option and variants to create these
	 */
	template <typename... Options>
		requires(std::is_base_of_v<option_base, Options> and ...)
	config &init(std::string_view usage, Options... options)
	{
		using std::operator""sv;

		m_sections.clear();

		m_usage = usage;
		m_ignore_unknown = false;

		add_section(""sv, std::forward<Options>(options)...);

		for (auto &f : get_section_factories())
		{
			std::unique_ptr<detail::section> sp(f->create());

			auto si = std::lower_bound(m_sections.begin(), m_sections.end(), sp->name(), [](const std::unique_ptr<detail::section> &s, std::string_view name)
				{ return s->name().compare(name) < 0; });

			if (si == m_sections.end())
				m_sections.insert(si, std::move(sp));
		}

		return *this;
	}

	/**
	 * @brief Extend a config instance with a set of \a options in a section called \a section_name
	 *
	 * @param section_name The name of the section to add to the config
	 * @param options Variadic list of options recognised by this config object, use mcfp::make_option and variants to create these
	 */
	template <typename... Options>
		requires(std::is_base_of_v<option_base, Options> and ...)
	config &add_section(std::string_view section_name, Options... options)
	{
		std::unique_ptr<detail::section> section(new detail::section(section_name, std::forward<Options>(options)...));

		auto si = std::lower_bound(m_sections.begin(), m_sections.end(), section_name, [](const std::unique_ptr<detail::section> &s, std::string_view name)
			{ return s->name().compare(name) < 0; });

		if (si != m_sections.end())
			si->reset(section.release());
		else
			m_sections.insert(si, std::move(section));
		
		return *this;
	}

	/**
	 * @brief Initialise a config instance with a \a usage message and a set of \a options
	 * in a so-called global section (no leading section name for the options when using get/has)
	 *
	 * @param usage The usage message
	 * @param options Variadic list of options recognised by this config object, use mcfp::make_option and variants to create these
	 */
	template <typename... Options>
		requires(std::is_base_of_v<option_base, Options> and ...)
	static void init_lib(std::string_view section_name, Options... options)
	{
		get_section_factories().emplace_back(new section_factory(section_name, std::forward<Options>(options)...));
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
	 * @brief Get the last option name, for use in error reporting
	 *
	 * @return std::string The last parsed or requested option
	 */
	std::string get_last_option() const
	{
		return get_last_option_storage();
	}

	/**
	 * @brief Simply return true if the option with \a name has a value assigned
	 *
	 * @param name The name of the option
	 * @return bool Returns true when the option has a value
	 */
	bool has(std::string_view name) const
	{
		auto opt = get_option(name);
		return opt != nullptr and (opt->m_seen > 0 or opt->m_default_value.has_value());
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
		auto opt = get_option(name);
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
			throw std::system_error(ec, "while getting option '" + std::string{ name } + '\'');

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

		// store name for inspection later on
		get_last_option_storage() = name;

		return_type result{};
		auto opt = get_option(name);

		if (opt == nullptr)
			ec = make_error_code(config_error::unknown_option);
		else
			result = opt->get_value<T>(ec);

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
		return m_operands;
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
		// Hack to be able to limit the width of the output (wrapping width)
		size_t terminal_width;
		if (auto sw = os.width(); sw != 0)
		{
			terminal_width = sw;
			os.width(0);
		}
		else
			terminal_width = get_terminal_width();

		if (not conf.m_usage.empty())
			os << conf.m_usage << '\n';

		size_t options_width = conf.get_option_width();

		if (options_width > terminal_width / 3)
			options_width = terminal_width / 3;

		if (options_width > 32)
			options_width = 32;

		if (options_width < 16)
			options_width = 16;

		for (auto &section : conf.m_sections)
			section->write(os, options_width, terminal_width);

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
		{
			if (get_last_option().empty())
				throw std::system_error(ec, "while parsing command line arguments");
			else
				throw std::system_error(ec, "while parsing command line arguments, option '" + get_last_option() + "'");
		}
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
		{
			std::string error_option = get_last_option();

			std::string file = has(config_option) ? get(config_option) : std::string{ config_file_name };

			if (get_last_option().empty())
				throw std::system_error(ec, "while parsing config file '" + file);
			else
				throw std::system_error(ec, "while parsing config file '" + file + "', option '" + error_option + "'");
		}
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
			VALUE,
			SECTION_START,
			SECTION_NAME,
			SECTION_NAME_END,
			SECTION_END
		} state = State::NAME_START;

		std::string section, name, value;

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
					else if (ch == '#' or ch == ';')
						state = State::COMMENT;
					else if (ch == '[')
						state = State::SECTION_START;
					else if (ch != ' ' and ch != '\t' and not is_eoln(ch))
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::COMMENT:
					if (is_eoln(ch))
						state = State::NAME_START;
					break;

				case State::SECTION_START:
					if (is_name_char(ch))
					{
						section = std::string{ (char)ch };
						state = State::SECTION_NAME;
					}
					else if (ch != ' ' and ch != '\t')
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::SECTION_NAME:
				case State::SECTION_NAME_END:
					if (is_name_char(ch) and state != State::SECTION_NAME_END)
						section += char(ch);
					else if (ch == ']')
						state = State::SECTION_END;
					else if (ch == ' ' or ch == '\t')
						state = State::SECTION_NAME_END;
					else
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::SECTION_END:
					if (is_eoln(ch))
						state = State::NAME_START;
					else
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::NAME:
					if (is_name_char(ch))
						name.insert(name.end(), static_cast<char>(ch));
					else if (is_eoln(ch))
					{
						// store name for inspection later on
						get_last_option_storage() = name;

						auto opt = get_option(section, name);

						if (opt == nullptr)
						{
							if (not m_ignore_unknown)
								ec = make_error_code(config_error::unknown_option);
						}
						else
							ec = make_error_code(config_error::missing_argument_for_option);

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
						ec = make_error_code(config_error::missing_argument_for_option);
					else if (ch != ' ' and ch != '\t')
						ec = make_error_code(config_error::invalid_config_file);
					break;

				case State::VALUE_START:
				case State::VALUE:
					if (is_eoln(ch))
					{
						auto opt = get_option(section, name);

						if (opt == nullptr)
						{
							if (not m_ignore_unknown)
								ec = make_error_code(config_error::unknown_option);
						}
						else if (opt->m_is_flag)
							opt->set_value(value, ec);
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

		m_operands.clear();

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
					m_operands.emplace_back(arg);
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
				m_operands.emplace_back(arg);
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

				// store name for inspection later on
				get_last_option_storage() = s_arg;

				opt = get_option(s_arg);
				if (opt == nullptr)
				{
					if (not m_ignore_unknown)
						ec = make_error_code(config_error::unknown_option);
					continue;
				}

				if (opt->m_is_flag)
				{
					if (opt_arg.empty() or opt_arg == "true")
						++opt->m_seen;
					else if (opt_arg == "false")
						opt->m_seen = 0;
					else
						ec = make_error_code(config_error::option_does_not_accept_argument);

					continue;
				}

				++opt->m_seen;
			}
			else // single character options
			{
				bool expect_option_argument = false;

				while (*arg != 0 and not ec)
				{
					// store name for inspection later on
					get_last_option_storage() = *arg;
					opt = get_option(*arg++);

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

	// --------------------------------------------------------------------

	constexpr static std::tuple<std::string_view, std::string_view> split_name(std::string_view name) noexcept
	{
		using std::operator""sv;

		auto p = name.find('.');
		return p == std::string_view::npos ? std::make_tuple(""sv, name) : std::make_tuple(name.substr(0, p), name.substr(p + 1));
	}

	// --------------------------------------------------------------------

  private:
	config() = default;
	config(const config &) = delete;
	config &operator=(const config &) = delete;

	/// @cond

	static std::string &get_last_option_storage()
	{
		thread_local static std::string s_last_option;
		return s_last_option;
	}

	// --------------------------------------------------------------------

	option_base *get_option(std::string_view section_name, std::string_view option_name) const
	{
		option_base *result = nullptr;

		for (auto &s : m_sections)
		{
			if (s->name() != section_name)
				continue;

			result = s->get_option(option_name);
			break;
		}

		return result;
	}

	option_base *get_option(std::string_view name) const
	{
		auto [section_name, option_name] = split_name(name);
		return get_option(section_name, option_name);
	}

	option_base *get_option(char short_name) const
	{
		option_base *result = nullptr;

		for (auto &s : m_sections)
		{
			result = s->get_option(short_name);

			if (result != nullptr)
				break;
		}

		return result;
	}

	size_t get_option_width() const
	{
		size_t result = 0;
		for (auto &s : m_sections)
		{
			auto w = s->get_option_width();
			if (result < w)
				result = w;
		}

		return result;
	}

	// --------------------------------------------------------------------

	class section_factory_base
	{
	  public:
		virtual ~section_factory_base() = default;

		virtual detail::section *create() const = 0;
	};

	template <typename... Options>
	class section_factory : public section_factory_base
	{
	  public:
		section_factory(std::string_view name, Options... options)
			: m_name(name)
			, m_options(std::forward<Options>(options)...)
		{
		}

		virtual detail::section *create() const
		{
			return std::apply([this](Options const &...opts)
				{ return new detail::section(m_name, opts...); }, m_options);
		}

		std::string m_name;
		std::tuple<Options...> m_options;
	};

	static std::vector<std::unique_ptr<const section_factory_base>> &get_section_factories()
	{
		static std::vector<std::unique_ptr<const section_factory_base>> s_factories;
		return s_factories;
	}

	// --------------------------------------------------------------------

	bool m_ignore_unknown = false;
	std::string m_usage;

	std::vector<std::string> m_operands;
	std::vector<std::unique_ptr<detail::section>> m_sections;

	/// @endcond
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
auto make_option(detail::ostring name, std::string_view description)
{
	return detail::option<T>(name.m_long, name.m_short, description, false);
}

template <typename T, std::enable_if_t<detail::is_container_type_v<T>, int> = 0>
auto make_option(detail::ostring name, std::string_view description)
{
	return detail::multiple_option<T>(name.m_long, name.m_short, description, false);
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
auto make_option(detail::ostring name, const T &v, std::string_view description)
{
	return detail::option<T>(name.m_long, name.m_short, v, description, false);
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
auto make_hidden_option(detail::ostring name, std::string_view description)
{
	return detail::option<T>(name.m_long, name.m_short, description, true);
}

template <typename T, std::enable_if_t<detail::is_container_type_v<T>, int> = 0>
auto make_hidden_option(detail::ostring name, std::string_view description)
{
	return detail::multiple_option<T>(name.m_long, name.m_short, description, true);
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
auto make_hidden_option(detail::ostring name, const T &v, std::string_view description)
{
	return detail::option<T>(name.m_long, name.m_short, v, description, true);
}

// --------------------------------------------------------------------
// To extend all configuration parameter lists with a default set handled
// by a library e.g.

#define MCFP_DEFINE_LIB_OPTIONS(LIB, SECTION, ...)        \
	const struct mcfp_lib_options                         \
	{                                                     \
		mcfp_lib_options()                                \
		{                                                 \
			mcfp::config::init_lib(SECTION, __VA_ARGS__); \
		}                                                 \
	} s_lib_options_for_lib_##LIB;

} // namespace mcfp

namespace std
{

template <>
struct is_error_condition_enum<mcfp::config_error>
	: public true_type
{
};

} // namespace std

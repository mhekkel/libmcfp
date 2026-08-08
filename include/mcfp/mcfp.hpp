// Copyright Maarten L. Hekkelman 2022-2025
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/// \file mcfp.hpp
/// This module library contains code to parse argc/argv and store the
/// values provided into a singleton object.

#ifndef MCFP_MODULE_MODE
# define MCFP_EXPORT
# define MCFP_INLINE inline

// IWYU pragma: begin_exports
# include "mcfp/error.hpp"
# include "mcfp/options.hpp"
# include "mcfp/sections.hpp"
# include "mcfp/text.hpp"
// IWYU pragma: end_exports

# include <algorithm>
# include <cassert>
# include <cstring>
# include <filesystem>
# include <memory>
# include <optional>
# include <system_error>
# include <type_traits>
# include <utility>
# include <vector>
#endif

namespace mcfp
{

// --------------------------------------------------------------------
/**
 * @brief This is the main interface to mcfp. It is a singleton class.
 * Use @ref mcfp::config::instance to create and/or
 * retrieve the single instance.
 *
 */

MCFP_EXPORT class config
{
  public:
	/**
	 * @brief Set the 'usage' string
	 *
	 * @param usage The usage message
	 */
	void set_usage(std::string usage)
	{
		m_usage = std::move(usage);
	}

	/**
	 * @brief Initialise a config instance with a \a usage message and a set of \a options
	 * in the global section (no leading section name for the options when using get or has)
	 *
	 * This method also initialises all predefined (library) sections.
	 *
	 * @param usage The usage message
	 * @param options Variadic list of options recognised by this config object, use mcfp::make_option and variants to create these
	 */
	template <typename... Options>
		requires(std::is_base_of_v<option_base, Options> and ...)
	config &init(std::string usage, Options &&...options)
	{
		m_sections.clear();

		m_usage = std::move(usage);
		m_ignore_unknown = false;

		add_section("", std::forward<Options>(options)...);

		for (auto &f : get_section_factories())
		{
			std::unique_ptr<section> sp(f->create());

			auto si = std::lower_bound(m_sections.begin(), m_sections.end(), sp->name(),
				[](const std::unique_ptr<section> &s, std::string_view name)
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
	config &add_section(const std::string &section_name, Options &&...options)
	{
		auto si = std::lower_bound(m_sections.begin(), m_sections.end(), section_name,
			[](const std::unique_ptr<section> &s, std::string_view name)
			{ return s->name().compare(name) < 0; });

		auto s = std::make_unique<section>(section_name, std::forward<Options>(options)...);

		if (si != m_sections.end() and (*si)->name() == section_name)
			*si = std::move(s);
		else
			m_sections.insert(si, std::move(s));

		return *this;
	}

	/**
	 * @brief Initialise a config instance with a \a usage message and a set of \a options
	 * in a secondary section named \a section_name. This configuration will be appended
	 * to the configuration specified in config::init.
	 *
	 * The reason for this functionality is to add configuration options to a program
	 * from e.g. a library, options the program itself is not aware of.
	 *
	 * @param section_name The name for the section/library
	 * @param options Variadic list of options recognised by this config object, use mcfp::make_option and variants to create these
	 */
	template <typename... Options>
		requires(std::is_base_of_v<option_base, Options> and ...)
	static void init_lib(std::string section_name, Options &&...options)
	{
		get_section_factories().emplace_back(new section_factory(std::move(section_name), std::forward<Options>(options)...));
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
	 * @brief Get the last parsed option name, for use in error reporting
	 *
	 * @return std::string The last parsed or requested option
	 */
	[[nodiscard]] std::string get_last_option() const
	{
		return s_last_option;
	}

	/**
	 * @brief Simply return true if the option with \a name has a value assigned
	 *
	 * @param name The name of the option
	 * @return bool Returns true when the option has a value
	 */
	[[nodiscard]] bool has(std::string_view name) const
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
	[[nodiscard]] int count(std::string_view name) const
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
	[[nodiscard]] auto get(std::string_view name) const
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
		s_last_option = name;

		return_type result{};
		auto opt = get_option(name);

		// if opt is null, the programmer has made an error requesting
		// an option that was not specified in the config::init call.
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
	[[nodiscard]] std::string get(std::string_view name) const
	{
		return get<std::string>(name);
	}

	/**
	 * @brief Returns the value for the option with name \a name
	 * wrapped in a std::optional<T> type.
	 *
	 * @tparam T The type of the value requested.
	 * @param name The name of the option requested
	 * @return std::optional<T> The value of the named option
	 */
	template <typename T>
	[[nodiscard]] auto get_optional(std::string_view name) const
	{
		using return_type = std::optional<std::remove_cv_t<T>>;

		std::error_code ec;
		return_type result = get<T>(name, ec);
		if (ec and ec != config_error::option_not_specified)
			throw std::system_error(ec, "while getting option '" + std::string{ name } + '\'');

		return result;
	}

	/**
	 * @brief Returns the value for the option with name \a name
	 * wrapped in a std::optional<std::string> type.
	 *
	 * @param name The name of the option requested
	 * @return std::optional<std::string> The value of the named option
	 */
	[[nodiscard]] auto get_optional(std::string_view name) const
	{
		return get_optional<std::string>(name);
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
	[[nodiscard]] std::string get(std::string_view name, std::error_code &ec) const
	{
		return get<std::string>(name, ec);
	}

	/**
	 * @brief Return the list of operands.
	 *
	 * @return const std::vector<std::string>& The operand as a vector of strings
	 */
	[[nodiscard]] const std::vector<std::string> &operands() const
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
	friend std::ostream &operator<<(std::ostream &os, const config &conf);

	// --------------------------------------------------------------------

	/**
	 * @brief Parse the \a argv vector containing \a argc elements. Throws
	 * an exception if any error was found
	 *
	 * @param argc The number of elements in \a argv
	 * @param argv The vector of command line arguments
	 */
	void parse(int argc, const char *const argv[]);

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
		std::initializer_list<std::string_view> search_dirs);

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
		std::initializer_list<std::string_view> search_dirs, std::error_code &ec);

	/**
	 * @brief Parse a configuration file specified by \a file
	 * If an error is found it is returned in the variable \a ec
	 *
	 * @param file The path to the config file
	 * @param ec The variable containing the error status
	 */
	void parse_config_file(const std::filesystem::path &file, std::error_code &ec);

  private:
	static constexpr bool is_name_char(int ch)
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
	void parse_config_file(std::istream &is, std::error_code &ec);

	/**
	 * @brief Parse the \a argv vector containing \a argc elements.
	 * In case of an error, the error is returned in \a ec
	 *
	 * @param argc The number of elements in \a argv
	 * @param argv The vector of command line arguments
	 * @param ec The variable receiving the error status
	 */
	void parse(int argc, const char *const argv[], std::error_code &ec);

	// --------------------------------------------------------------------
	/// @cond

	constexpr static std::tuple<std::string_view, std::string_view> split_name(std::string_view name) noexcept
	{
		auto p = name.find('.');
		return p == std::string_view::npos
		           ? std::make_tuple(std::string_view{}, name)
		           : std::make_tuple(name.substr(0, p), name.substr(p + 1));
	}

	// --------------------------------------------------------------------

  public:
	config(const config &) = delete;
	config &operator=(const config &) = delete;

  private:
	config() = default;

	// --------------------------------------------------------------------

	[[nodiscard]] option_base *get_option(std::string_view section_name, std::string_view option_name) const
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

	[[nodiscard]] option_base *get_option(std::string_view name) const
	{
		auto [section_name, option_name] = split_name(name);
		return get_option(section_name, option_name);
	}

	[[nodiscard]] option_base *get_option(char short_name) const
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

	[[nodiscard]] size_t get_option_width() const
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

		[[nodiscard]] virtual section *create() const = 0;
	};

	template <typename... Options>
	class section_factory : public section_factory_base
	{
	  public:
		explicit section_factory(std::string name, Options &&...options)
			: m_name(std::move(name))
			, m_options(std::forward<Options>(options)...)
		{
		}

		[[nodiscard]] section *create() const override
		{
			return std::apply([this](Options const &...opts)
				{ return new section(m_name, opts...); }, m_options);
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
	std::vector<std::unique_ptr<section>> m_sections;

	static thread_local std::string s_last_option;

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
MCFP_EXPORT template <typename T = void>
auto make_option(ostring name, std::string description)
	requires(not is_container_type_v<T>)
{
	return option<T>(name.m_long, name.m_short, std::move(description), false);
}

/** @cond */
MCFP_EXPORT template <typename T>
auto make_option(ostring name, std::string description)
	requires(is_container_type_v<T>)
{
	return multiple_option<T>(name.m_long, name.m_short, std::move(description), false);
}
/** @endcond */

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
MCFP_EXPORT template <typename T>
auto make_option(ostring name, const T &v, std::string description)
	requires(not is_container_type_v<T>)
{
	return option<T>(name.m_long, name.m_short, v, std::move(description), false);
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
MCFP_EXPORT template <typename T = void>
auto make_hidden_option(ostring name, std::string description)
	requires(not is_container_type_v<T>)
{
	return option<T>(name.m_long, name.m_short, description, true);
}

/** @cond */
MCFP_EXPORT template <typename T>
auto make_hidden_option(ostring name, std::string description)
	requires(is_container_type_v<T>)
{
	return multiple_option<T>(name.m_long, name.m_short, description, true);
}
/** @endcond */

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
MCFP_EXPORT template <typename T>
auto make_hidden_option(ostring name, const T &v, std::string description)
	requires(not is_container_type_v<T>)
{
	return option<T>(name.m_long, name.m_short, v, description, true);
}

} // namespace mcfp

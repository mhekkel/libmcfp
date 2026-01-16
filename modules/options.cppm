/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2025 Maarten L. hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

module;

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

namespace mcfp
{

/// @cond
// --------------------------------------------------------------------
// Some template wizardry to detect containers, needed to have special
// handling of options that can be repeated.

template <typename T>
using iterator_t = typename T::iterator;

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using std_string_npos_t = decltype(T::npos);

/**
 * @brief Template to detect whether a type is a container
 */

template <typename T, typename = void>
struct is_container_type : std::false_type
{
};

template <typename T>
	requires(is_detected_v<value_type_t, T> and
			 is_detected_v<iterator_t, T> and
			 not is_detected_v<std_string_npos_t, T>)
struct is_container_type<T>
	: std::true_type
{
};

template <typename T>
inline constexpr bool is_container_type_v = is_container_type<T>::value;

static_assert(is_container_type_v<std::vector<int>>);
static_assert(is_container_type_v<std::vector<std::string>>);

// --------------------------------------------------------------------
// Some helper classes, to allow compile time checking of options strings

// This error reporting function is not constexpr and thus when it is
// called by the checking the format of options strings, it will cause
// a compile time error.
[[noreturn]] inline void report_error(const char *msg)
{
	(void)fputs(msg, stderr);
	exit(1);
}

// --------------------------------------------------------------------

template <typename CharT>
class string_view_base
{
  public:
	using char_type = CharT;

	using value_type = char_type;
	using iterator = const char_type *;

	constexpr explicit string_view_base(const char *s) noexcept
		: m_data(s)
	{
		while (m_data[m_size] != 0)
			++m_size;
	}

	constexpr string_view_base(const char *s, size_t N) noexcept
		: m_data(s)
		, m_size(N)
	{
	}

	template <size_t N>
	constexpr explicit string_view_base(const char (&s)[N]) noexcept
		: m_data(s)
		, m_size(N - 1)
	{
	}

	constexpr string_view_base() noexcept = default;
	constexpr string_view_base(const string_view_base &) noexcept = default;
	constexpr string_view_base &
	operator=(const string_view_base &) noexcept = default;
	// constexpr string_view_base(nullptr_t) = delete;

	template <typename StringType>
	// requires(std::is_same_v<typename StringType::value_type, value_type>)
	constexpr explicit string_view_base(const StringType &s) noexcept
		: m_data(s.data())
		, m_size(s.size())
	{
	}

	[[nodiscard]] constexpr const char_type *data() const noexcept { return m_data; }
	[[nodiscard]] constexpr size_t size() const noexcept { return m_size; }

	[[nodiscard]] constexpr iterator begin() const noexcept { return m_data; }
	[[nodiscard]] constexpr iterator end() const noexcept { return m_data + m_size; }
	[[nodiscard]] constexpr char_type operator[](size_t ix) const noexcept
	{
		return m_data[ix];
	}

	[[nodiscard]] constexpr char_type front() const noexcept { return m_data[0]; }
	[[nodiscard]] constexpr char_type back() const noexcept { return m_data[m_size - 1]; }

	[[nodiscard]] constexpr string_view_base substr(size_t pos, size_t len) const noexcept
	{
		return { m_data + pos, len };
	}

  private:
	const char_type *m_data = nullptr;
	size_t m_size = 0;
};

using string_view = string_view_base<char>;

// --------------------------------------------------------------------
// A parsed options string, that is, split out the short and long names

struct ostring
{
	string_view m_str;
	string_view m_long;
	string_view m_short;

	template <size_t N>
	consteval inline ostring(const char (&s)[N]) // NOLINT(hicpp-explicit-conversions)
		: m_str(s, N - 1)
	{
		parse();
	}

	constexpr void parse();
};

constexpr inline bool is_alnum(int ch) noexcept
{
	return (ch >= '0' and ch <= '9') or (ch >= 'a' and ch <= 'z') or
	       (ch >= 'A' and ch <= 'Z');
}

constexpr inline bool is_valid_option_char(char ch) noexcept
{
	return ch == '-' or ch == '_' or is_alnum(ch);
}

constexpr void ostring::parse()
{
	if (m_str.size() < 1)
		report_error("Empty string is not allowed for an option");

	if (m_str.front() == '-')
		report_error("Option strings should not start with a hyphen");

	auto len = m_str.size();

	if (m_str.size() == 1)
	{
		if (not is_alnum(m_str.front()))
			report_error("Single character options should be alnum");

		m_long = m_short = m_str;
	}
	else
	{
		if (m_str.size() > 2 and m_str[m_str.size() - 2] == ',')
		{
			if (not is_alnum(m_str.back()))
				report_error("Short variant of option should be alnum");

			m_short = m_str.substr(m_str.size() - 1, 1);
			len -= 2;
		}

		for (size_t ix = 0; auto ch : m_str)
		{
			if (ix++ == len)
				break;

			if (not is_valid_option_char(ch))
				report_error("Short variant of option should be alnum");
		}

		m_long = m_str.substr(0, len);
	}
}

// --------------------------------------------------------------------
// The options classes

// The option traits classes are used to convert from the string-based
// command line argument to the type that should be stored.
// In fact, here is where the command line arguments are checked for
// proper formatting.
export template <typename T, typename = void>
struct option_traits;

template <typename T>
	requires(std::is_arithmetic_v<T>)
struct option_traits<T>
{
	using value_type = T;

	static value_type set_value(std::string_view argument, std::error_code &ec)
	{
		value_type value{};
		auto r =
			from_chars(argument.data(), argument.data() + argument.length(), value);
		if (r.ec != std::errc())
			ec = std::make_error_code(r.ec);
		else if (*r.ptr != 0)
			ec = std::make_error_code(std::errc::invalid_argument);
		return value;
	}

	static std::string to_string(const T &value)
	{
		char b[32];
		auto r = std::to_chars(b, b + sizeof(b), value);
		if (r.ec != std::errc())
			throw std::system_error(std::make_error_code(r.ec));
		return { b, r.ptr };
	}
};

template <>
struct option_traits<std::filesystem::path>
{
	using value_type = std::filesystem::path;

	static value_type set_value(std::string_view argument,
		std::error_code & /*ec*/)
	{
		return value_type{ argument };
	}

	static std::string to_string(const std::filesystem::path &value)
	{
		return value.string();
	}
};

template <typename T>
	requires(not std::is_arithmetic_v<T> and std::is_assignable_v<std::string, T>)
struct option_traits<T>
{
	using value_type = std::string;

	static value_type set_value(std::string_view argument,
		std::error_code & /*ec*/)
	{
		return value_type{ argument };
	}

	static std::string to_string(const T &value) { return { value }; }
};

// The Options. The reason to have this weird constructing of
// polymorphic options based on templates is to have a very
// simple interface. The disadvantage is that the options have
// to be copied during the construction of the config object.

struct option_base
{
	std::string m_name;    ///< The long argument name
	std::string m_desc;    ///< The description of the argument
	char m_short_name;     ///< The single character name of the argument, can be zero
	bool m_is_flag = true, ///< When true, this option does not allow arguments
		m_multi = false,   ///< When true, this option allows mulitple values.
		m_hidden;          ///< When true, this option is hidden from the help text
	int m_seen = 0;        ///< How often the option was seen on the command line

	// We store the actual data in the argument list, i.e. strings
	std::vector<std::string> m_value;
	std::optional<std::string> m_default_value;

	option_base(const option_base &rhs) = default;

	constexpr option_base(string_view name_long, string_view name_short,
		std::string desc, bool hidden)
		: m_name(name_long.begin(), name_long.end())
		, m_desc(std::move(desc))
		, m_short_name(name_short.size() > 0 ? name_short.front() : 0)
		, m_hidden(hidden)
	{
	}

	virtual ~option_base() = default;

	virtual void set_value(std::string_view /*value*/,
		std::error_code & /*ec*/) = 0;

	template <typename T>
	T get_value(std::error_code &ec) const
	{
		T result{};

		if (m_value.empty())
		{
			if (m_default_value)
			{
				if constexpr (is_container_type_v<T>)
					result.emplace_back(option_traits<typename T::value_type>::set_value(
						*m_default_value, ec));
				else
					result = option_traits<T>::set_value(*m_default_value, ec);
			}
			else if constexpr (not is_container_type_v<T>) // Return an empty list if not specified
				ec = make_error_code(config_error::option_not_specified);
		}
		else
		{
			if constexpr (is_container_type_v<T>)
			{
				for (auto &a : m_value)
				{
					result.emplace_back(
						option_traits<typename T::value_type>::set_value(a, ec));
					if (ec)
					{
						result.clear();
						break;
					}
				}
			}
			else
				result = option_traits<T>::set_value(m_value.front(), ec);
		}

		return result;
	}

	[[nodiscard]] size_t width(std::string_view section_name) const;
	void write(std::ostream &os, std::string_view section_name, size_t indent, size_t output_width) const;
};

template <typename T>
struct option : public option_base
{
	using traits_type = option_traits<T>;
	using value_type = typename option_traits<T>::value_type;

	option(const option &rhs) = default;

	option(string_view name_long, string_view name_short, std::string desc,
		bool hidden)
		: option_base(name_long, name_short, std::move(desc), hidden)
	{
		m_is_flag = false;
	}

	option(string_view name_long, string_view name_short,
		const value_type &default_value, std::string desc, bool hidden)
		: option(name_long, name_short, std::move(desc), hidden)
	{
		if constexpr (std::is_same_v<value_type, std::string>)
			m_default_value = default_value;
		else
			m_default_value = traits_type::to_string(default_value);
	}

	void set_value(std::string_view argument, std::error_code &ec) override
	{
		traits_type::set_value(argument, ec);
		if (not ec)
		{
			m_value.clear();
			m_value.emplace_back(argument);
		}
	}
};

template <typename T>
struct multiple_option : public option_base
{
	using value_type = typename T::value_type;
	using traits_type = option_traits<value_type>;

	multiple_option(const multiple_option &rhs) = default;

	multiple_option(string_view name_long, string_view name_short,
		std::string desc, bool hidden)
		: option_base(name_long, name_short, std::move(desc), hidden)
	{
		m_is_flag = false;
		m_multi = true;
	}

	void set_value(std::string_view argument, std::error_code &ec) override
	{
		traits_type::set_value(argument, ec);
		if (not ec)
			m_value.emplace_back(argument);
	}
};

template <>
struct option<void> : public option_base
{
	option(const option &rhs) = default;

	option(string_view name_long, string_view name_short, std::string desc,
		bool hidden)
		: option_base(name_long, name_short, std::move(desc), hidden)
	{
	}

	void set_value(std::string_view value, std::error_code &ec) override
	{
		if (value == "true")
			m_seen = 1;
		else if (value == "false")
			m_seen = 0;
		else if (auto [ptr, ec2] = mcfp::from_chars(
					 value.data(), value.data() + value.length(), m_seen);
			ec2 != std::errc{} or ptr != value.data() + value.length())
			ec = make_error_code(config_error::wrong_type_cast_flag);
	}
};

/// @endcond


} // namespace mcfp
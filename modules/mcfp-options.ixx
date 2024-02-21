/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Maarten L. hekkelman
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

#include <any>
#include <cassert>
#include <charconv>
#include <cmath>
#include <compare>
#include <experimental/type_traits>
#include <filesystem>
#include <string>
#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

export module mcfp:options;

import :error;
import :text;
import :utilities;

namespace mcfp
{

// --------------------------------------------------------------------
// Some template wizardry to detect containers, needed to have special
// handling of options that can be repeated.

template <typename T>
using iterator_t = typename T::iterator;

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using std_string_npos_t = decltype(T::npos);

template <typename T, typename = void>
struct is_container_type : std::false_type
{
};

/**
 * @brief Template to detect whether a type is a container
 */

template <typename T>
struct is_container_type<T,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not std::experimental::is_detected_v<std_string_npos_t, T>>> : std::true_type
{
};

template <typename T>
constexpr bool is_container_type_v = is_container_type<T>::value;

// --------------------------------------------------------------------
// The options classes

// The option traits classes are used to convert from the string-based
// command line argument to the type that should be stored.
// In fact, here is where the command line arguments are checked for
// proper formatting.
template <typename T, typename = void>
struct option_traits;


// So sad, libc++ has no std::from_chars for floating point types
template <typename T>
using from_chars_function = decltype(std::from_chars(std::declval<const char *>(), std::declval<const char *>(), std::declval<T &>()));

template <typename T>
requires std::is_arithmetic_v<T> and std::experimental::is_detected_v<from_chars_function, T>
struct option_traits<T>
{
	using value_type = T;

	static value_type set_value(std::string_view argument, std::error_code &ec)
	{
		value_type value{};
		auto r = std::from_chars(argument.data(), argument.data() + argument.length(), value);
		if (r.ec != std::errc())
			ec = std::make_error_code(r.ec);
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

// Since libc++ does not have std::from_chars for floating points, we have to write it ourselves
template <std::floating_point T>
std::from_chars_result from_chars(const char *first, const char *last, T &value)
{
	using value_type = T;

	std::from_chars_result result{ first, {} };

	enum State
	{
		IntegerSign,
		Integer,
		Fraction,
		ExponentSign,
		Exponent
	} state = IntegerSign;
	int sign = 1;
	unsigned long long vi = 0;
	long double f = 1;
	int exponent_sign = 1;
	int exponent = 0;
	bool done = false;

	while (not done and result.ec == std::errc())
	{
		char ch = result.ptr != last ? *result.ptr : 0;
		++result.ptr;

		switch (state)
		{
			case IntegerSign:
				if (ch == '-')
				{
					sign = -1;
					state = Integer;
				}
				else if (ch == '+')
					state = Integer;
				else if (ch >= '0' and ch <= '9')
				{
					vi = ch - '0';
					state = Integer;
				}
				else if (ch == '.')
					state = Fraction;
				else
					result.ec = std::errc::invalid_argument;
				break;

			case Integer:
				if (ch >= '0' and ch <= '9')
					vi = 10 * vi + (ch - '0');
				else if (ch == 'e' or ch == 'E')
					state = ExponentSign;
				else if (ch == '.')
					state = Fraction;
				else
				{
					done = true;
					--result.ptr;
				}
				break;

			case Fraction:
				if (ch >= '0' and ch <= '9')
				{
					vi = 10 * vi + (ch - '0');
					f /= 10;
				}
				else if (ch == 'e' or ch == 'E')
					state = ExponentSign;
				else
				{
					done = true;
					--result.ptr;
				}
				break;

			case ExponentSign:
				if (ch == '-')
				{
					exponent_sign = -1;
					state = Exponent;
				}
				else if (ch == '+')
					state = Exponent;
				else if (ch >= '0' and ch <= '9')
				{
					exponent = ch - '0';
					state = Exponent;
				}
				else
					result.ec = std::errc::invalid_argument;
				break;

			case Exponent:
				if (ch >= '0' and ch <= '9')
					exponent = 10 * exponent + (ch - '0');
				else
				{
					done = true;
					--result.ptr;
				}
				break;
		}
	}

	if (result.ec == std::errc())
	{
		long double v = f * vi * sign;
		if (exponent != 0)
			v *= std::pow(10, exponent * exponent_sign);

		if (std::isnan(v))
			result.ec = std::errc::invalid_argument;
		else if (std::abs(v) > std::numeric_limits<value_type>::max())
			result.ec = std::errc::result_out_of_range;

		value = static_cast<value_type>(v);
	}

	return result;
}

template <std::floating_point T>
requires (std::experimental::is_detected_v<from_chars_function, T> == false)
struct option_traits<T>
{
	using value_type = T;

	static value_type set_value(std::string_view argument, std::error_code &ec)
	{
		value_type value{};
		auto r = mcfp::from_chars(argument.data(), argument.data() + argument.length(), value);
		if (r.ec != std::errc())
			ec = std::make_error_code(r.ec);
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

	static value_type set_value(std::string_view argument, std::error_code & /*ec*/)
	{
		return value_type{ argument };
	}

	static std::string to_string(const std::filesystem::path &value)
	{
		return value.string();
	}
};

template <typename T>
requires (not std::is_arithmetic_v<T> and std::is_assignable_v<std::string, T>)
struct option_traits<T>
{
	using value_type = std::string;

	static value_type set_value(std::string_view argument, std::error_code & /*ec*/)
	{
		return value_type{ argument };
	}

	static std::string to_string(const T &value)
	{
		return { value };
	}
};

// The Options. The reason to have this weird constructing of
// polymorphic options based on templates is to have a very
// simple interface. The disadvantage is that the options have
// to be copied during the construction of the config object.

struct option_base
{
	std::string m_name;        ///< The long argument name
	std::string m_desc;        ///< The description of the argument
	char m_short_name;         ///< The single character name of the argument, can be zero
	bool m_is_flag = true,     ///< When true, this option does not allow arguments
		m_has_default = false, ///< When true, this option has a default value.
		m_multi = false,       ///< When true, this option allows mulitple values.
		m_hidden;              ///< When true, this option is hidden from the help text
	int m_seen = 0;            ///< How often the option was seen on the command line

	option_base(const option_base &rhs) = default;

	option_base(std::string_view name, std::string_view desc, bool hidden)
		: m_name(name)
		, m_desc(desc)
		, m_short_name(0)
		, m_hidden(hidden)
	{
		if (m_name.length() == 1)
			m_short_name = m_name.front();
		else if (m_name.length() > 2 and m_name[m_name.length() - 2] == ',')
		{
			m_short_name = m_name.back();
			m_name.erase(m_name.end() - 2, m_name.end());
		}
	}

	virtual ~option_base() = default;

	virtual void set_value(std::string_view /*value*/, std::error_code & /*ec*/)
	{
		assert(false);
	}

	virtual std::any get_value() const
	{
		return {};
	}

	virtual std::string get_default_value() const
	{
		return {};
	}

	size_t width() const
	{
		size_t result = m_name.length();
		if (result <= 1)
			result = 2;
		else if (m_short_name != 0)
			result += 7;
		if (not m_is_flag)
		{
			result += 4;
			if (m_has_default)
				result += 4 + get_default_value().length();
		}
		return result + 6;
	}

	void write(std::ostream &os, size_t width) const
	{
		if (m_hidden) // quick exit
			return;

		size_t w2 = 2;
		os << "  ";
		if (m_short_name)
		{
			os << '-' << m_short_name;
			w2 += 2;
			if (m_name.length() > 1)
			{
				os << " [ --" << m_name << " ]";
				w2 += 7 + m_name.length();
			}
		}
		else
		{
			os << "--" << m_name;
			w2 += 2 + m_name.length();
		}

		if (not m_is_flag)
		{
			os << " arg";
			w2 += 4;

			if (m_has_default)
			{
				auto default_value = get_default_value();
				os << " (=" << default_value << ')';
				w2 += 4 + default_value.length();
			}
		}

		auto leading_spaces = width;
		if (w2 + 2 > width)
			os << std::endl;
		else
			leading_spaces = width - w2;

		word_wrapper ww(m_desc, get_terminal_width() - width);
		for (auto line : ww)
		{
			os << std::string(leading_spaces, ' ') << line << std::endl;
			leading_spaces = width;
		}
	}
};

template <typename T>
struct option : public option_base
{
	using traits_type = option_traits<T>;
	using value_type = typename option_traits<T>::value_type;

	std::optional<value_type> m_value;

	option(const option &rhs) = default;

	option(std::string_view name, std::string_view desc, bool hidden)
		: option_base(name, desc, hidden)
	{
		m_is_flag = false;
	}

	option(std::string_view name, const value_type &default_value, std::string_view desc, bool hidden)
		: option(name, desc, hidden)
	{
		m_has_default = true;
		m_value = default_value;
	}

	void set_value(std::string_view argument, std::error_code &ec) override
	{
		m_value = traits_type::set_value(argument, ec);
	}

	std::any get_value() const override
	{
		std::any result;
		if (m_value)
			result = *m_value;
		return result;
	}

	std::string get_default_value() const override
	{
		if constexpr (std::is_same_v<value_type, std::string>)
			return *m_value;
		else
			return traits_type::to_string(*m_value);
	}
};

template <typename T>
struct multiple_option : public option_base
{
	using value_type = typename T::value_type;
	using traits_type = option_traits<value_type>;

	std::vector<value_type> m_values;

	multiple_option(const multiple_option &rhs) = default;

	multiple_option(std::string_view name, std::string_view desc, bool hidden)
		: option_base(name, desc, hidden)
	{
		m_is_flag = false;
		m_multi = true;
	}

	void set_value(std::string_view argument, std::error_code &ec) override
	{
		m_values.emplace_back(traits_type::set_value(argument, ec));
	}

	std::any get_value() const override
	{
		return { m_values };
	}
};

template <>
struct option<void> : public option_base
{
	option(const option &rhs) = default;

	option(std::string_view name, std::string_view desc, bool hidden)
		: option_base(name, desc, hidden)
	{
	}
};

} // namespace mcfp
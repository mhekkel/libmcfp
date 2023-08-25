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

#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string>
#include <vector>

#if __has_include(<experimental/type_traits>)
#include <experimental/type_traits>
#else
#include <type_traits>
#endif

namespace mcfp::detail
{

#if (not defined(__cpp_lib_experimental_detect) or (__cpp_lib_experimental_detect < 201505)) and (not defined(_LIBCPP_VERSION) or _LIBCPP_VERSION < 5000)
// This code is copied from:
// https://ld2015.scusa.lsu.edu/cppreference/en/cpp/experimental/is_detected.html

template< class... >
using void_t = void;

namespace detail
{
	template <class Default, class AlwaysVoid,
			template<class...> class Op, class... Args>
	struct detector
	{
		using value_t = std::false_type;
		using type = Default;
	};
	
	template <class Default, template<class...> class Op, class... Args>
	struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
		// Note that std::void_t is a c++17 feature
		using value_t = std::true_type;
		using type = Op<Args...>;
	};
} // namespace detail

struct nonesuch
{
	nonesuch() = delete;
	~nonesuch() = delete;
	nonesuch(nonesuch const&) = delete;
	void operator=(nonesuch const&) = delete;
};

template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template<class...> class Op, class... Args>
constexpr inline bool is_detected_v = is_detected<Op,Args...>::value;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template<class...> class Op, class... Args>
constexpr inline bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

#else

template <template<class...> class Op, class... Args>
constexpr inline bool is_detected_v = std::experimental::is_detected<Op,Args...>::value;

#endif

template <typename T>
struct my_charconv
{
	using value_type = T;

	static std::from_chars_result from_chars(const char *first, const char *last, value_type &value)
	{
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

	template <typename Iterator, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	static std::to_chars_result to_chars(Iterator first, Iterator last, const T &value)
	{
		int size = last - first;
		int r;

		if constexpr (std::is_same_v<T, long double>)
			r = snprintf(first, last - first, "%lg", value);
		else
			r = snprintf(first, last - first, "%g", value);

		std::to_chars_result result;
		if (r < 0 or r >= size)
			result = { first, std::errc::value_too_large };
		else
			result = { first + r, std::errc() };

		return result;
	}
};

template <typename T>
struct std_charconv
{
	static std::from_chars_result from_chars(const char *a, const char *b, T &d)
	{
		return std::from_chars(a, b, d);
	}

	template<typename Iterator>
	static std::to_chars_result to_chars(Iterator a, Iterator b, const T &value)
	{
		return std::to_chars(a, b, value);
	}
};

template <typename T>
using from_chars_function = decltype(std::from_chars(std::declval<const char *>(), std::declval<const char *>(), std::declval<T &>()));

template <typename T>
using charconv = typename std::conditional_t<is_detected_v<from_chars_function, T>, std_charconv<T>, my_charconv<T>>;

}
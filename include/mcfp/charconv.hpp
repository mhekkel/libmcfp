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

#ifndef MCFP_EXPORT
# error "Please include mcfp.hpp only"
#endif

#ifndef IN_MODULE_INTERFACE

# include <charconv>
# if __has_include(<experimental/type_traits>)
#  include <experimental/type_traits>
# endif
# include <type_traits>
# include <utility>

#endif

namespace mcfp
{

/// @cond

#if (not defined(__cpp_lib_experimental_detect) or (__cpp_lib_experimental_detect < 201505)) and (not defined(_LIBCPP_VERSION) or _LIBCPP_VERSION < 5000)
// This code is copied from:
// https://ld2015.scusa.lsu.edu/cppreference/en/cpp/experimental/is_detected.html

template <class...>
using void_t = void;

namespace detail
{
	template <class Default, class AlwaysVoid,
		template <class...> class Op, class... Args>
	struct detector
	{
		using value_t = std::false_type;
		using type = Default;
	};

	template <class Default, template <class...> class Op, class... Args>
	struct detector<Default, void_t<Op<Args...>>, Op, Args...>
	{
		// Note that std::void_t is a c++17 feature
		using value_t = std::true_type;
		using type = Op<Args...>;
	};
} // namespace detail

struct nonesuch
{
	nonesuch() = delete;
	~nonesuch() = delete;
	nonesuch(nonesuch const &) = delete;
	void operator=(nonesuch const &) = delete;
};

MCFP_EXPORT template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

MCFP_EXPORT template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

MCFP_EXPORT template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

MCFP_EXPORT template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

MCFP_EXPORT template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

MCFP_EXPORT template <class Expected, template <class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;
#else

MCFP_EXPORT template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = std::experimental::is_detected<Op, Args...>::value;

#endif

template <typename T>
using from_chars_function = decltype(std::from_chars(std::declval<const char *>(), std::declval<const char *>(), std::declval<T &>()));

template <typename T>
struct std_charconv
{
	static std::from_chars_result from_chars(const char *a, const char *b, T &d)
	{
		return std::from_chars(a, b, d);
	}
};

template <typename T, typename = void>
struct ff_charconv;

template <typename T>
	requires(std::is_floating_point_v<T>)
struct ff_charconv<T>
{
	static std::from_chars_result from_chars(const char *a, const char *b, T &v);
};

MCFP_EXPORT template <typename T>
using charconv = typename std::conditional_t<is_detected_v<from_chars_function, T>, std_charconv<T>, ff_charconv<T>>;

MCFP_EXPORT template <typename T>
constexpr auto from_chars(const char *s, const char *e, T &v)
{
	return charconv<T>::from_chars(s, e, v);
}

/// @endcond

} // namespace mcfp
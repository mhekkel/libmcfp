//          Copyright Maarten L. Hekkelman 2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "mcfp/mcfp.hpp"

#include "fast_float/fast_float.h"

#include <charconv>

namespace mcfp
{

template <>
std::from_chars_result ff_charconv<float>::from_chars(const char *a, const char *b, float &v)
{
	auto r = fast_float::from_chars(a, b, v);
	return { r.ptr, r.ec };
}

template <>
std::from_chars_result ff_charconv<double>::from_chars(const char *a, const char *b, double &v)
{
	auto r = fast_float::from_chars(a, b, v);
	return { r.ptr, r.ec };
}

} // namespace mcfp

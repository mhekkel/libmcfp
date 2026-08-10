// Copyright Maarten L. Hekkelman 2025
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MCFP_MODULE_MODE
# include "fast_float/fast_float.h"
# include "mcfp/mcfp.hpp"

# include <charconv>
#endif

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

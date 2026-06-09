// Copyright Maarten L. Hekkelman 2022-2025
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifndef MCFP_EXPORT
# define MCFP_EXPORT
# define MCFP_INLINE inline
#endif

/**
 * @file text.hpp
 * This file contains word wrapping code
 */

#ifndef IN_MODULE_INTERFACE
# include <cstddef>
# include <string_view>
# include <vector>
#endif

namespace mcfp
{

/// @cond

// --------------------------------------------------------------------
/// Simplified line breaking code taken from a decent text editor.
/// In this case, simplified means it only supports ASCII.
/// The algorithm uses dynamic programming to find the optimal
/// separation in lines.

MCFP_EXPORT class word_wrapper : public std::vector<std::string_view>
{
  public:
	word_wrapper(std::string_view text, size_t width);

  private:
	std::vector<std::string_view> wrap_line(std::string_view line, size_t width);

	std::string_view::const_iterator next_line_break(std::string_view::const_iterator text,
		std::string_view::const_iterator end);
};

/// @endcond

} // namespace mcfp

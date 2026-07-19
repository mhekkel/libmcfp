// Copyright Maarten L. Hekkelman 2022-2025
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * @file text.hpp
 * This file contains word wrapping code
 */

#ifndef MCFP_MODULE_MODE
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

MCFP_EXPORT class word_wrapper
{
  public:
	using const_iterator = std::vector<std::string_view>::const_iterator;

	word_wrapper(std::string_view text, size_t width);

	[[nodiscard]] const_iterator begin() const noexcept { return m_lines.begin(); }
	[[nodiscard]] const_iterator end() const noexcept { return m_lines.end(); }

  private:
	std::vector<std::string_view> wrap_line(std::string_view line, size_t width);

	std::string_view::const_iterator next_line_break(std::string_view::const_iterator text,
		std::string_view::const_iterator end);

	std::vector<std::string_view> m_lines;
};

/// @endcond

} // namespace mcfp

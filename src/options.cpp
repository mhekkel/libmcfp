/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Maarten L. Hekkelman
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

// #ifdef BUILD_CXX_MODULE
// module;
// #else
# include "mcfp/mcfp.hpp"
// #endif

# include <ostream>
# include <utility>

#ifdef BUILD_CXX_MODULE
module mcfp;
#endif

namespace mcfp
{

size_t option_base::width(std::string_view section_name) const
{
	size_t result = m_name.length();
	if (not section_name.empty())
		result += section_name.length() + 1;
	if (result <= 1)
		result = 2;
	else if (m_short_name != 0 and section_name.empty())
		result += 7;
	if (not m_is_flag)
	{
		result += 4;
		if (m_default_value.has_value())
			result += 4 + m_default_value->length();
	}
	return result + 6;
}

void option_base::write(std::ostream &os, std::string_view section_name, size_t indent, size_t output_width) const
{
	if (m_hidden) // quick exit
		return;

	size_t w2 = 2;
	if (section_name.empty())
	{
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
	}
	else
	{
		os << "  ";
		os << "--" << section_name << "." << m_name;
		w2 += 2 + section_name.length() + 1 + m_name.length();
	}

	if (not m_is_flag)
	{
		os << " arg";
		w2 += 4;

		if (m_default_value.has_value())
		{
			auto default_value = *m_default_value;
			os << " (=" << default_value << ')';
			w2 += 4 + default_value.length();
		}
	}

	std::string indent_str(indent, ' ');
	bool do_indent = false;

	if (w2 + 2 > indent)
	{
		os << '\n';
		do_indent = true;
	}
	else
		os << indent_str.substr(0, indent - w2);

	word_wrapper ww(m_desc, output_width - indent - 1);
	for (auto line : ww)
	{
		if (std::exchange(do_indent, true))
			os << indent_str;

		while (not line.empty() and std::isspace(line.back()))
			line.remove_suffix(1);

		os << line << '\n';
	}
}

} // namespace mcfp

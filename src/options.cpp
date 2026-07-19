// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MCFP_MODULE_MODE
# include "mcfp/mcfp.hpp"

# include <ostream>
# include <utility>
#endif

namespace mcfp
{

std::size_t option_base::width(std::string_view section_name) const
{
	std::size_t result = m_name.length();
	if (not section_name.empty())
		result += section_name.length() + 1;
	if (result < 2)
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

void option_base::write(std::ostream &os, std::string_view section_name, std::size_t indent, std::size_t output_width) const
{
	if (m_hidden) // quick exit
		return;

	std::size_t w2 = 2;
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

// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MCFP_MODULE_MODE
# include "mcfp/mcfp.hpp"

# include <algorithm>
# include <cctype>
# include <cstdint>
# include <limits>
# include <string_view>
# include <vector>
#endif

namespace mcfp
{

// --------------------------------------------------------------------
/// Simplified line breaking code taken from a decent text editor.
/// In this case, simplified means it only supports ASCII.
/// The algorithm uses dynamic programming to find the optimal
/// separation in lines.

word_wrapper::word_wrapper(std::string_view text, std::size_t width)
{
	std::string_view::size_type line_start = 0, line_end = text.find('\n');

	for (;;)
	{
		auto line = text.substr(line_start, line_end - line_start);
		if (line.empty())
			m_lines.push_back(line);
		else
		{
			auto lines = wrap_line(line, width);
			m_lines.insert(m_lines.end(), lines.begin(), lines.end());
		}

		if (line_end == std::string_view::npos)
			break;

		line_start = line_end + 1;
		line_end = text.find('\n', line_start);
	}
}

std::vector<std::string_view> word_wrapper::wrap_line(std::string_view line, std::size_t width)
{
	std::vector<std::string_view> result;
	std::vector<std::size_t> offsets = { 0 };

	auto b = line.begin();
	while (b != line.end())
	{
		auto e = next_line_break(b, line.end());

		offsets.push_back(e - line.begin());

		b = e;
	}

	std::size_t count = offsets.size() - 1;

	std::vector<std::size_t> minima(count + 1, std::numeric_limits<std::size_t>::max());
	minima[0] = 0;
	std::vector<std::size_t> breaks(count + 1, 0);

	for (std::size_t i = 0; i < count; ++i)
	{
		std::size_t j = i + 1;
		while (j <= count)
		{
			std::size_t w = offsets[j] - offsets[i];

			if (w > width)
				break;

			while (w > 0 and std::isspace(line[offsets[i] + w - 1]))
				--w;

			std::size_t cost = minima[i];
			if (j < count and width > w) // last line may be shorter
				cost += (width - w) * (width - w);

			if (cost < minima[j])
			{
				minima[j] = cost;
				breaks[j] = i;
			}

			++j;
		}
	}

	std::size_t j = count;
	while (j > 0)
	{
		std::size_t i = breaks[j];
		result.push_back(line.substr(offsets[i], offsets[j] - offsets[i]));
		j = i;
	}

	std::ranges::reverse(result);

	return result;
}

std::string_view::const_iterator word_wrapper::next_line_break(std::string_view::const_iterator text, std::string_view::const_iterator end)
{
	if (text == end)
		return text;

	enum LineBreakClass
	{
		OP, // OpenPunctuation,
		CL, // ClosePunctuation,
		CP, // CloseParenthesis,
		QU, // Quotation,
		EX, // Exlamation,
		SY, // SymbolAllowingBreakAfter,
		IS, // InfixNumericSeparator,
		PR, // PrefixNumeric,
		PO, // PostfixNumeric,
		NU, // Numeric,
		AL, // Alphabetic,
		HY, // Hyphen,
		BA, // BreakAfter,
		CM, // CombiningMark,
		WJ, // WordJoiner,

		MB, // MandatoryBreak,
		SP, // Space,
	};

	static const LineBreakClass kASCII_LineBreakTable[128] = {
		CM, CM, CM, CM, CM, CM, CM, CM,
		CM, BA, MB, MB, MB, SP, CM, CM,
		CM, CM, CM, CM, CM, CM, CM, CM,
		CM, CM, CM, CM, CM, CM, CM, CM,
		SP, EX, QU, AL, PR, PO, AL, QU,
		OP, CP, AL, PR, IS, HY, IS, SY,
		NU, NU, NU, NU, NU, NU, NU, NU,
		NU, NU, IS, IS, AL, AL, AL, EX,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, OP, PR, CP, AL, AL,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, AL, AL, AL, AL, AL,
		AL, AL, AL, OP, BA, CL, AL, CM
	};

	enum BreakAction
	{
		DBK = 0, // direct break 	(blank in table)
		IBK,     // indirect break	(% in table)
		PBK,     // prohibited break (^ in table)
		CIB,     // combining indirect break
		CPB      // combining prohibited break
	};

	static const BreakAction brkTable[15][15] = {
		//         OP   CL   CP   QU   EX   SY   IS   PR   PO   NU   AL   HY   BA   CM   WJ
		/* OP */ { PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, CPB, PBK },
		/* CL */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, IBK, IBK, DBK, DBK, IBK, IBK, CIB, PBK },
		/* CP */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* QU */ { PBK, PBK, PBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* EX */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, IBK, IBK, CIB, PBK },
		/* SY */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, DBK, IBK, IBK, CIB, PBK },
		/* IS */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* PR */ { IBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* PO */ { IBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* NU */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* AL */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* HY */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, DBK, IBK, IBK, CIB, PBK },
		/* BA */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, IBK, IBK, CIB, PBK },
		/* CM */ { DBK, PBK, PBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, IBK, CIB, PBK },
		/* WJ */ { IBK, PBK, PBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, CIB, PBK },
	};

	std::uint8_t ch = *text;

	LineBreakClass cls;

	if (ch < 128)
		cls = kASCII_LineBreakTable[ch];
	else
		cls = AL;

	if (cls == SP)
		cls = WJ;

	LineBreakClass ncls = cls;

	while (++text != end and cls != MB)
	{
		ch = *text;

		LineBreakClass lcls = ncls;

		if (ch < 128)
			ncls = kASCII_LineBreakTable[ch];
		else
			ncls = AL;

		if (ncls == MB)
		{
			++text;
			break;
		}

		if (ncls == SP)
			continue;

		BreakAction brk = brkTable[cls][ncls];

		if (brk == DBK or (brk == IBK and lcls == SP))
			break;

		cls = ncls;
	}

	return text;
}

} // namespace mcfp

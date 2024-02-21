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

#include <algorithm>
#include <string>
#include <vector>
#include <limits>

/**
 * @file text.hpp
 * This file contains an implementation of charconv and of work wrapping code
 */

export module mcfp:text;

namespace mcfp
{

// --------------------------------------------------------------------
/// Simplified line breaking code taken from a decent text editor.
/// In this case, simplified means it only supports ASCII.
/// The algorithm uses dynamic programming to find the optimal
/// separation in lines.

export class word_wrapper : public std::vector<std::string_view>
{
  public:
	word_wrapper(std::string_view text, size_t width)
		: m_width(width)
	{
		std::string_view::size_type line_start = 0, line_end = text.find('\n');
		
		for (;;)
		{
			auto line = text.substr(line_start, line_end - line_start);
			if (line.empty())
				this->push_back(line);
			else
			{
				auto lines = wrap_line(line);
				this->insert(this->end(), lines.begin(), lines.end());
			}

			if (line_end == std::string_view::npos)
				break;

			line_start = line_end + 1;
			line_end = text.find('\n', line_start);
		}
	}

  private:
	std::vector<std::string_view> wrap_line(std::string_view line)
	{
		std::vector<std::string_view> result;
		std::vector<size_t> offsets = { 0 };

		auto b = line.begin();
		while (b != line.end())
		{
			auto e = next_line_break(b, line.end());

			offsets.push_back(e - line.begin());

			b = e;
		}

		size_t count = offsets.size() - 1;

		std::vector<size_t> minima(count + 1, std::numeric_limits<size_t>::max());
		minima[0] = 0;
		std::vector<size_t> breaks(count + 1, 0);

		for (size_t i = 0; i < count; ++i)
		{
			size_t j = i + 1;
			while (j <= count)
			{
				size_t w = offsets[j] - offsets[i];

				if (w > m_width)
					break;

				while (w > 0 and std::isspace(line[offsets[i] + w - 1]))
					--w;

				size_t cost = minima[i];
				if (j < count) // last line may be shorter
					cost += (m_width - w) * (m_width - w);

				if (cost < minima[j])
				{
					minima[j] = cost;
					breaks[j] = i;
				}

				++j;
			}
		}

		size_t j = count;
		while (j > 0)
		{
			size_t i = breaks[j];
			result.push_back(line.substr(offsets[i], offsets[j] - offsets[i]));
			j = i;
		}

		reverse(result.begin(), result.end());

		return result;
	}

	std::string_view::const_iterator next_line_break(std::string_view::const_iterator text, std::string_view::const_iterator end)
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

		uint8_t ch = *text;

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

	size_t m_width;
};

} // namespace mcfp

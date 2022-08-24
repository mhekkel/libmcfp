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

#include <cmath>
#include <charconv>
#include <experimental/type_traits>

namespace cfg
{

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
};

template <typename T>
struct std_charconv
{
	static std::from_chars_result from_chars(const char *a, const char *b, T &d)
	{
		return std::from_chars(a, b, d);
	}
};

template <typename T>
using from_chars_function = decltype(std::from_chars(std::declval<const char *>(), std::declval<const char *>(), std::declval<T &>()));

template <typename T>
using charconv = typename std::conditional_t<std::experimental::is_detected_v<from_chars_function, T>, std_charconv<T>, my_charconv<T>>;

// --------------------------------------------------------------------
// word wrapping code, simplified to the max


// --------------------------------------------------------------------
// Simplified line breaking code taken from a decent text editor.
// In this case, simplified means it only supports ASCII.

enum LineBreakClass
{
	kLBC_OpenPunctuation,
	kLBC_ClosePunctuation,
	kLBC_CloseParenthesis,
	kLBC_Quotation,
	// kLBC_NonBreaking,
	// kLBC_Nonstarter,
	kLBC_Exlamation,
	kLBC_SymbolAllowingBreakAfter,
	// kLBC_InfixNumericSeparator,
	kLBC_PrefixNumeric,
	kLBC_PostfixNumeric,
	kLBC_Numeric,
	kLBC_Alphabetic,
	// kLBC_Ideographic,
	// kLBC_Inseperable,
	kLBC_Hyphen,
	kLBC_BreakAfter,
	// kLBC_BreakBefor,
	// kLBC_BreakOpportunityBeforeAndAfter,
	// kLBC_ZeroWidthSpace,
	kLBC_CombiningMark,
	kLBC_WordJoiner,
	// kLBC_HangulLVSyllable,
	// kLBC_HangulLVTSyllable,
	// kLBC_HangulLJamo,
	// kLBC_HangulVJamo,
	// kLBC_HangulTJamo,

	// kLBC_MandatoryBreak,
	kLBC_CarriageReturn,
	kLBC_LineFeed,
	// kLBC_NextLine,
	// kLBC_Surrogate,
	kLBC_Space,
	// kLBC_ContigentBreakOpportunity,
	// kLBC_Ambiguous,
	// kLBC_ComplexContext,
	kLBC_Unknown
};

const LineBreakClass kASCII_LBTable[128] =
	{
		kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark,
		kLBC_CombiningMark, kLBC_BreakAfter, kLBC_LineFeed, kLBC_MandatoryBreak, kLBC_MandatoryBreak, kLBC_CarriageReturn, kLBC_CombiningMark, kLBC_CombiningMark,
		kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark,
		kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark, kLBC_CombiningMark,
		kLBC_Space, kLBC_Exlamation, kLBC_Quotation, kLBC_Alphabetic, kLBC_PrefixNumeric, kLBC_PostfixNumeric, kLBC_Alphabetic, kLBC_Quotation,
		kLBC_OpenPunctuation, kLBC_CloseParenthesis, kLBC_Alphabetic, kLBC_PrefixNumeric,

		// comma treated differently here, it is not a numeric separator in PDB
		kLBC_SymbolAllowingBreakAfter /*	kLBC_InfixNumericSeparator */,

		kLBC_Hyphen, kLBC_InfixNumericSeparator, kLBC_SymbolAllowingBreakAfter,
		kLBC_Numeric, kLBC_Numeric, kLBC_Numeric, kLBC_Numeric, kLBC_Numeric, kLBC_Numeric, kLBC_Numeric, kLBC_Numeric,
		kLBC_Numeric, kLBC_Numeric, kLBC_InfixNumericSeparator, kLBC_InfixNumericSeparator, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Exlamation,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_OpenPunctuation, kLBC_PrefixNumeric, kLBC_CloseParenthesis, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic,
		kLBC_Alphabetic, kLBC_Alphabetic, kLBC_Alphabetic, kLBC_OpenPunctuation, kLBC_BreakAfter, kLBC_ClosePunctuation, kLBC_Alphabetic, kLBC_CombiningMark};

std::string::const_iterator nextLineBreak(std::string::const_iterator text, std::string::const_iterator end)
{
	if (text == end)
		return text;

	enum breakAction
	{
		DBK = 0, // direct break 	(blank in table)
		IBK,     // indirect break	(% in table)
		PBK,     // prohibited break (^ in table)
		CIB,     // combining indirect break
		CPB      // combining prohibited break
	};

	const breakAction brkTable[27][27] = {
		//        OP   CL   CP   QU   GL   NS   EX   SY   IS   PR   PO   NU   AL   ID   IN   HY   BA   BB   B2   ZW   CM   WJ   H2   H3   JL   JV   JT
		/* OP */ {PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, PBK, CPB, PBK, PBK, PBK, PBK, PBK, PBK},
		/* CL */ {DBK, PBK, PBK, IBK, IBK, PBK, PBK, PBK, PBK, IBK, IBK, DBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* CP */ {DBK, PBK, PBK, IBK, IBK, PBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* QU */ {PBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, IBK},
		// /* GL */ {IBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, IBK},
		// /* NS */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* EX */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* SY */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		// /* IS */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* PR */ {IBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, IBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, IBK},
		/* PO */ {IBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* NU */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* AL */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		// /* ID */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		// /* IN */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* HY */ {DBK, PBK, PBK, IBK, DBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* BA */ {DBK, PBK, PBK, IBK, DBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, DBK, DBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		// /* BB */ {IBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, IBK},
		// /* B2 */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, DBK, DBK, DBK, DBK, IBK, IBK, DBK, PBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		// /* ZW */ {DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK, PBK, DBK, DBK, DBK, DBK, DBK, DBK, DBK},
		/* CM */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, DBK, IBK, IBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, DBK},
		/* WJ */ {IBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, IBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, IBK},
		// /* H2 */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, IBK, IBK},
		// /* H3 */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, IBK},
		// /* JL */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, IBK, IBK, IBK, IBK, DBK},
		// /* JV */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, IBK, IBK},
		// /* JT */ {DBK, PBK, PBK, IBK, IBK, IBK, PBK, PBK, PBK, DBK, IBK, DBK, DBK, DBK, IBK, IBK, IBK, DBK, DBK, PBK, CIB, PBK, DBK, DBK, DBK, DBK, IBK},
	};

	uint8_t ch = static_cast<uint8_t>(*text);

	LineBreakClass cls;

	if (ch == '\n')
		cls = kLBC_MandatoryBreak;
	else if (ch < 128)
	{
		cls = kASCII_LBTable[ch];
		if (cls > kLBC_MandatoryBreak and cls != kLBC_Space) // duh...
			cls = kLBC_Alphabetic;
	}
	else
		cls = kLBC_Unknown;

	if (cls == kLBC_Space)
		cls = kLBC_WordJoiner;

	LineBreakClass ncls = cls;

	while (++text != end and cls != kLBC_MandatoryBreak)
	{
		ch = *text;

		LineBreakClass lcls = ncls;

		if (ch == '\n')
		{
			++text;
			break;
		}

		ncls = kASCII_LBTable[ch];

		if (ncls == kLBC_Space)
			continue;

		breakAction brk = brkTable[cls][ncls];

		if (brk == DBK or (brk == IBK and lcls == kLBC_Space))
			break;

		cls = ncls;
	}

	return text;
}

std::vector<std::string> wrapLine(const std::string &text, size_t width)
{
	std::vector<std::string> result;
	std::vector<size_t> offsets = {0};

	auto b = text.begin();
	while (b != text.end())
	{
		auto e = nextLineBreak(b, text.end());

		offsets.push_back(e - text.begin());

		b = e;
	}

	size_t count = offsets.size() - 1;

	std::vector<size_t> minima(count + 1, 1000000);
	minima[0] = 0;
	std::vector<size_t> breaks(count + 1, 0);

	for (size_t i = 0; i < count; ++i)
	{
		size_t j = i + 1;
		while (j <= count)
		{
			size_t w = offsets[j] - offsets[i];

			if (w > width)
				break;

			while (w > 0 and isspace(text[offsets[i] + w - 1]))
				--w;

			size_t cost = minima[i];
			if (j < count) // last line may be shorter
				cost += (width - w) * (width - w);

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
		result.push_back(text.substr(offsets[i], offsets[j] - offsets[i]));
		j = i;
	}

	reverse(result.begin(), result.end());

	return result;
}

std::vector<std::string> word_wrap(const std::string &text, size_t width)
{
	std::vector<std::string> result;
	for (auto p : cif::split<std::string>(text, "\n"))
	{
		if (p.empty())
		{
			result.push_back("");
			continue;
		}

		auto lines = wrapLine(p, width);
		result.insert(result.end(), lines.begin(), lines.end());
	}

	return result;
}




} // namespace cfg

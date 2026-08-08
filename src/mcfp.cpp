// Copyright Maarten L. Hekkelman 2022-2025
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MCFP_MODULE_MODE
# include "mcfp/mcfp.hpp"

# include <cassert>
# include <climits>
# include <cstdint>
# include <filesystem>
# include <fstream>
# include <ostream>

# if __has_include(<sys/ioctl.h>)
// #  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <unistd.h>
# elif defined(_WIN32)
#  include <Windows.h>
#  include <cstdio>
#  include <io.h>
# endif
#endif

namespace mcfp
{

#if defined(_WIN32)
/// @brief Get the width in columns of the current terminal
/// @return number of columns of the terminal
uint32_t get_terminal_width()
{
	uint32_t result = 80;

	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	if (::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		result = csbi.srWindow.Right - csbi.srWindow.Left + 1;

	return result;
}

#elif __has_include(<sys/ioctl.h>)
/// @brief Get the width in columns of the current terminal
/// @return number of columns of the terminal
std::uint32_t get_terminal_width()
{
	std::uint32_t result = 80;

	if (::isatty(STDOUT_FILENO))
	{
		struct winsize w{};
		::ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // NOLINT(hicpp-vararg)
		result = w.ws_col;
	}
	return result;
}
#else
# warning "Could not find the terminal width, falling back to default"
MCFP_INLINE std::uint32_t get_terminal_width()
{
	return 80;
}
#endif

// --------------------------------------------------------------------

thread_local std::string config::s_last_option;

void config::parse(int argc, const char *const argv[])
{
	std::error_code ec;
	parse(argc, argv, ec);
	if (ec)
	{
		if (get_last_option().empty())
			throw std::system_error(ec, "while parsing command line arguments");
		else
			throw std::system_error(ec, "while parsing command line arguments, option '" + get_last_option() + "'");
	}
}

void config::parse(int argc, const char *const argv[], std::error_code &ec)
{
	using namespace std::literals;

	m_operands.clear();

	enum class State
	{
		options,
		operands
	} state = State::options;

	for (int i = 1; i < argc and not ec; ++i)
	{
		const char *arg = argv[i];

		if (arg == nullptr) // should not happen
			break;

		if (state == State::options)
		{
			if (*arg != '-') // according to POSIX this is the end of options, start operands
			                 // state = State::operands;
			{                // however, people nowadays expect to be able to mix operands and options
				m_operands.emplace_back(arg);
				continue;
			}
			else if (arg[1] == '-' and arg[2] == 0)
			{
				state = State::operands;
				continue;
			}
		}

		if (state == State::operands)
		{
			m_operands.emplace_back(arg);
			continue;
		}

		option_base *opt = nullptr;
		std::string_view opt_arg;

		assert(*arg == '-');
		++arg;

		if (*arg == '-') // double --, start of new argument
		{
			++arg;

			assert(*arg != 0); // this should not happen, as it was checked for before

			std::string_view s_arg(arg);
			std::string_view::size_type p = s_arg.find('=');

			if (p != std::string_view::npos)
			{
				opt_arg = s_arg.substr(p + 1);
				s_arg = s_arg.substr(0, p);
			}

			// store name for inspection later on
			s_last_option = s_arg;

			opt = get_option(s_arg);
			if (opt == nullptr)
			{
				if (not m_ignore_unknown)
					ec = make_error_code(config_error::unknown_option);
				continue;
			}

			if (opt->m_is_flag)
			{
				if (opt_arg.empty())
					++opt->m_seen;
				else
					opt->set_value(opt_arg, ec);

				continue;
			}

			++opt->m_seen;
		}
		else // single character options
		{
			bool expect_option_argument = false;

			while (*arg != 0 and not ec)
			{
				// store name for inspection later on
				s_last_option = *arg;
				opt = get_option(*arg++);

				if (opt == nullptr)
				{
					if (not m_ignore_unknown)
						ec = make_error_code(config_error::unknown_option);
					continue;
				}

				++opt->m_seen;
				if (opt->m_is_flag)
					continue;

				opt_arg = arg;
				expect_option_argument = true;
				break;
			}

			if (not expect_option_argument)
				continue;
		}

		if (opt_arg.empty() and i + 1 < argc) // So, the = character was not present, the next arg must be the option argument
		{
			++i;
			opt_arg = argv[i];
		}

		if (opt_arg.empty())
			ec = make_error_code(config_error::missing_argument_for_option);
		else
			opt->set_value(opt_arg, ec);
	}
}

void config::parse_config_file(std::string_view config_option, std::string_view config_file_name,
	std::initializer_list<std::string_view> search_dirs)
{
	std::error_code ec;
	parse_config_file(config_option, config_file_name, search_dirs, ec);
	if (ec)
	{
		std::string error_option = get_last_option();

		std::string file = has(config_option) ? get(config_option) : std::string{ config_file_name };

		if (get_last_option().empty())
			throw std::system_error(ec, "while parsing config file '" + file);
		else
			throw std::system_error(ec, "while parsing config file '" + file + "', option '" + error_option + "'");
	}
}

void config::parse_config_file(const std::filesystem::path &file, std::error_code &ec)
{
	std::ifstream is(file);
	if (is.is_open())
		parse_config_file(is, ec);
}

void config::parse_config_file(std::string_view config_option, std::string_view config_file_name,
	std::initializer_list<std::string_view> search_dirs, std::error_code &ec)
{
	std::string file_name{ config_file_name };
	bool parsed_config_file = false;

	if (has(config_option))
		file_name = get<std::string>(config_option);

	for (std::filesystem::path dir : search_dirs)
	{
		std::ifstream file(dir / file_name);

		if (not file.is_open())
			continue;

		parse_config_file(file, ec);
		parsed_config_file = true;
		break;
	}

	if (not parsed_config_file and has(config_option))
		ec = make_error_code(config_error::config_file_not_found);
}

void config::parse_config_file(std::istream &is, std::error_code &ec)
{
	auto &buffer = *is.rdbuf();

	enum class State
	{
		NAME_START,
		COMMENT,
		NAME,
		ASSIGN,
		VALUE_START,
		VALUE,
		SECTION_START,
		SECTION_NAME,
		SECTION_NAME_END,
		SECTION_END
	} state = State::NAME_START;

	std::string section, name, value;

	for (;;)
	{
		auto ch = buffer.sbumpc();

		switch (state)
		{
			case State::NAME_START:
				if (is_name_char(ch))
				{
					name = { static_cast<char>(ch) };
					value.clear();
					state = State::NAME;
				}
				else if (ch == '#' or ch == ';')
					state = State::COMMENT;
				else if (ch == '[')
					state = State::SECTION_START;
				else if (ch != ' ' and ch != '\t' and not is_eoln(ch))
					ec = make_error_code(config_error::invalid_config_file);
				break;

			case State::COMMENT:
				if (is_eoln(ch))
					state = State::NAME_START;
				break;

			case State::SECTION_START:
				if (is_name_char(ch))
				{
					section = std::string{ static_cast<char>(ch) };
					state = State::SECTION_NAME;
				}
				else if (ch != ' ' and ch != '\t')
					ec = make_error_code(config_error::invalid_config_file);
				break;

			case State::SECTION_NAME:
			case State::SECTION_NAME_END:
				if (is_name_char(ch) and state != State::SECTION_NAME_END)
					section += static_cast<char>(ch);
				else if (ch == ']')
					state = State::SECTION_END;
				else if (ch == ' ' or ch == '\t')
					state = State::SECTION_NAME_END;
				else
					ec = make_error_code(config_error::invalid_config_file);
				break;

			case State::SECTION_END:
				if (is_eoln(ch))
					state = State::NAME_START;
				else
					ec = make_error_code(config_error::invalid_config_file);
				break;

			case State::NAME:
				if (is_name_char(ch))
					name.insert(name.end(), static_cast<char>(ch));
				else if (is_eoln(ch))
				{
					// store name for inspection later on
					s_last_option = name;

					auto opt = get_option(section, name);

					if (opt == nullptr)
					{
						if (not m_ignore_unknown)
							ec = make_error_code(config_error::unknown_option);
					}
					else
						ec = make_error_code(config_error::missing_argument_for_option);

					state = State::NAME_START;
				}
				else
				{
					buffer.sungetc();
					state = State::ASSIGN;
				}
				break;

			case State::ASSIGN:
				if (ch == '=')
					state = State::VALUE_START;
				else if (is_eoln(ch))
					ec = make_error_code(config_error::missing_argument_for_option);
				else if (ch != ' ' and ch != '\t')
					ec = make_error_code(config_error::invalid_config_file);
				break;

			case State::VALUE_START:
			case State::VALUE:
				if (is_eoln(ch))
				{
					auto opt = get_option(section, name);

					if (opt == nullptr)
					{
						if (not m_ignore_unknown)
							ec = make_error_code(config_error::unknown_option);
					}
					else if (opt->m_is_flag)
						opt->set_value(value, ec);
					else if (not value.empty() and (opt->m_seen == 0 or opt->m_multi))
					{
						opt->set_value(value, ec);
						++opt->m_seen;
					}

					state = State::NAME_START;
				}
				else if (state == State::VALUE)
					value.insert(value.end(), static_cast<char>(ch));
				else if (ch != ' ' and ch != '\t')
				{
					value = { static_cast<char>(ch) };
					state = State::VALUE;
				}
				break;
		}

		if (ec or ch == std::char_traits<char>::eof())
			break;
	}
}

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const config &conf)
{
	// Hack to be able to limit the width of the output (wrapping width)
	size_t terminal_width;
	if (auto sw = os.width(); sw != 0)
	{
		terminal_width = sw;
		os.width(0);
	}
	else
		terminal_width = get_terminal_width();

	if (not conf.m_usage.empty())
		os << conf.m_usage << '\n';

	size_t options_width = conf.get_option_width();

	if (options_width > terminal_width / 3)
		options_width = terminal_width / 3;

	if (options_width > 32)
		options_width = 32;

	if (options_width < 16)
		options_width = 16;

	for (auto &section : conf.m_sections)
		section->write(os, options_width, terminal_width);

	return os;
}

} // namespace mcfp

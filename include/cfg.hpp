/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Maarten L. Hekkelman
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

#include <cstring>

#include <charconv>
#include <deque>
#include <memory>
#include <optional>

namespace cfg
{

enum class config_error
{
	unknown_option = 1,
	no_parameter,
	invalid_parameter_type,
	invalid_argument,
	option_does_not_accept_argument,
	missing_argument_for_option,
	option_not_specified,
};

class config_category_impl : public std::error_category
{
  public:
	const char *name() const noexcept override
	{
		return "configuration";
	}

	std::string message(int ev) const override
	{
		switch (static_cast<config_error>(ev))
		{
			case config_error::unknown_option:
				return "unknown option";
			case config_error::no_parameter:
				return "no parameter";
			case config_error::invalid_parameter_type:
				return "invalid parameter type";
			case config_error::invalid_argument:
				return "invalid argument";
			case config_error::option_does_not_accept_argument:
				return "option does not accept argument";
			case config_error::missing_argument_for_option:
				return "missing argument for option";
			case config_error::option_not_specified:
				return "option was not specified";
			default:
				assert(false);
				return "unknown error code";
		}
	}

	bool equivalent(const std::error_code &code, int condition) const noexcept override
	{
		return false;
	}
};

inline std::error_category &config_category()
{
	static config_category_impl instance;
	return instance;
}

inline std::error_code make_error_code(config_error e)
{
	return std::error_code(static_cast<int>(e), config_category());
}

inline std::error_condition make_error_condition(config_error e)
{
	return std::error_condition(static_cast<int>(e), config_category());
}

// --------------------------------------------------------------------

class config
{
  public:
	struct option_base
	{
		std::string m_name;
		char m_short_name;
		int m_seen = 0;

		option_base(const option_base &rhs) = default;

		option_base(std::string_view name)
			: m_name(name)
			, m_short_name(0)
		{
			if (m_name.length() == 1)
			{
				m_short_name = m_name.front();
				m_name.clear();
			}
			else if (m_name.length() > 2 and m_name[m_name.length() - 2] == ',')
			{
				m_short_name = m_name.back();
				m_name.erase(m_name.end() - 2, m_name.end());
			}
		}

		virtual ~option_base() = default;
		virtual option_base *clone() = 0;
		virtual bool is_flag() const = 0;

		virtual void set_value(std::string_view argument, std::error_code &ec)
		{
			assert(false);
		}
	};

	template <typename T, typename = void>
	struct option : public option_base
	{
		std::optional<T> m_value;

		option(const option &rhs) = default;

		option(std::string_view name)
			: option_base(name)
		{
		}

		option(std::string_view name, const T &v)
			: option_base(name)
			, m_value(v)
		{
		}

		option_base *clone() override
		{
			return new option(*this);
		}

		bool is_flag() const override { return false; }

		void set_value(std::string_view argument, std::error_code &ec) override;
	};

	template <typename... Options>
	void init(Options... options)
	{
		m_impl.reset(new config_impl(std::forward<Options>(options)...));
	}

	void parse(int argc, const char *const argv[])
	{
		std::error_code ec;
		parse(argc, argv, ec);
		if (ec)
			throw std::system_error(ec);
	}

	void parse(int argc, const char *const argv[], std::error_code &ec)
	{
		using namespace std::literals;

		enum class State { options, operands } state = State::options;

		for (int i = 1; i < argc and not ec; ++i)
		{
			const char *arg = argv[i];

			if (arg == nullptr)	// should not happen
				break;

			if (state == State::options)
			{
				if (*arg != '-')	// end of options, start operands
					state = State::operands;
				else if (arg[1] == '-' and arg[2] == 0)
				{
					state = State::operands;
					continue;
				}
			}

			if (state == State::operands)
			{
				m_impl->m_operands.emplace_back(arg);
				continue;
			}

			option_base *opt = nullptr;
			std::string_view opt_arg;

			assert(*arg == '-');
			++arg;

			if (*arg == '-') // double --, start of new argument
			{
				++arg;

				assert(*arg != 0);	 // this should not happen, as it was checked for before

				std::string_view s_arg(arg);
				std::string_view::size_type p = s_arg.find('=');

				if (p != std::string_view::npos)
				{
					opt_arg = s_arg.substr(p + 1);
					s_arg = s_arg.substr(0, p);
				}

				opt = m_impl->get_option(s_arg);
				if (opt == nullptr)
				{
					if (not m_ignore_unknown_options)
						ec = make_error_code(config_error::unknown_option);
					continue;
				}

				if (opt->is_flag())
				{
					if (not opt_arg.empty())
						ec = make_error_code(config_error::option_does_not_accept_argument);

					++opt->m_seen;
					continue;
				}

				++opt->m_seen;
			}
			else	// single character options
			{
				bool expect_option_argument = false;

				while (*arg != 0 and not ec)
				{
					opt = m_impl->get_option(*arg++);

					if (opt == nullptr)
					{
						if (not m_ignore_unknown_options)
							ec = make_error_code(config_error::unknown_option);
						continue;
					}

					++opt->m_seen;
					if (opt->is_flag())
						continue;

					opt_arg = arg;
					expect_option_argument = true;
					break;
				}

				if (not expect_option_argument)
					continue;
			}

			if (opt_arg.empty() and i + 1 < argc)	// So, the = character was not present, the next arg must be the option argument
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

	static config &instance()
	{
		static std::unique_ptr<config> s_instance;
		if (not s_instance)
			s_instance.reset(new config);
		return *s_instance;
	}

	bool has(std::string_view name) const
	{
		auto opt = m_impl->get_option(name);
		return opt != nullptr and opt->m_seen > 0;
	}

	int count(std::string_view name) const
	{
		auto opt = m_impl->get_option(name);
		return opt ? opt->m_seen : 0;
	}

	template <typename T>
	auto get(const std::string &name) const
	{
		auto opt = m_impl->get_option(name);
		if (opt == nullptr)
			throw std::system_error(make_error_code(config_error::unknown_option), name);

		auto v_opt = dynamic_cast<option<T> *>(opt);
		if (v_opt == nullptr)
			throw std::system_error(make_error_code(config_error::invalid_parameter_type), name);

		if (not v_opt->m_value)
			throw std::system_error(make_error_code(config_error::option_not_specified), name);

		return *v_opt->m_value;
	}

	const std::vector<std::string> &operands() const
	{
		return m_impl->m_operands;
	}

  private:
	config() = default;
	config(const config &) = delete;
	config &operator=(const config &) = delete;

	struct config_impl
	{
		// virtual ~config_impl_base() = default;

		template <typename... Options>
		config_impl(Options... options)
		{
			(m_options.push_back(options.clone()), ...);
		}

		~config_impl()
		{
			for (auto opt : m_options)
				delete opt;
		}

		option_base *get_option(std::string_view name) const
		{
			for (auto &o : m_options)
			{
				if (o->m_name == name)
					return &*o;
			}
			return nullptr;
		}

		option_base *get_option(char short_name) const
		{
			for (auto &o : m_options)
			{
				if (o->m_short_name == short_name)
					return &*o;
			}
			return nullptr;
		}

		std::vector<option_base *> m_options;
		std::vector<std::string> m_operands;
	};

	std::unique_ptr<config_impl> m_impl;
	bool m_ignore_unknown_options = false;
};

template <>
struct config::option<void> : public config::option_base
{
	option(const option &rhs) = default;

	option(std::string_view name)
		: option_base(name)
	{
	}

	option_base *clone() override
	{
		return new option(*this);
	}

	bool is_flag() const override { return true; }
};

template <>
void config::option<int>::set_value(std::string_view argument, std::error_code &ec)
{
	int value;
	auto r = std::from_chars(argument.begin(), argument.end(), value);
	if (r.ec != std::errc())
		ec = std::make_error_code(r.ec);
	m_value = value;
}

template <>
void config::option<std::filesystem::path>::set_value(std::string_view argument, std::error_code &ec)
{
	m_value = std::filesystem::path{ argument };
}

template <typename T = void>
auto make_option(std::string_view name)
{
	return config::option<T>(name);
}

template <typename T>
auto make_option(std::string_view name, const T &v)
{
	return config::option<T>(name, v);
}

} // namespace cfg

namespace std
{

template <>
struct is_error_condition_enum<cfg::config_error>
	: public true_type
{
};

} // namespace std
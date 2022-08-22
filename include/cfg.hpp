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

class config_option_error : public std::runtime_error
{
  public:
	config_option_error(const std::string &err)
		: runtime_error(err)
	{
	}
};

class unknown_option_error : public std::runtime_error
{
  public:
	unknown_option_error(const std::string &err)
		: runtime_error(err)
	{
	}
};

class no_param_error : public std::runtime_error
{
  public:
	no_param_error(const std::string &err)
		: runtime_error(err)
	{
	}
};

class invalid_param_type_error : public std::runtime_error
{
  public:
	invalid_param_type_error(const std::string &err)
		: runtime_error(err)
	{
	}
};

class invalid_argument_error : public std::runtime_error
{
  public:
	invalid_argument_error(const std::string &err)
		: runtime_error(err)
	{
	}
};

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

		virtual void set_value(std::string_view argument)
		{
			assert(false);
		}
	};

	template <typename T>
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

		void set_value(std::string_view argument) override;
	};

	template <typename... Options>
	void init(Options... options)
	{
		m_impl.reset(new config_impl(std::forward<Options>(options)...));
	}

	void parse(int argc, const char *const argv[])
	{
		using namespace std::literals;

		for (int i = 1; i < argc; ++i)
		{
			const char *arg = argv[i];

			if (arg == nullptr)
				break;

			std::deque<option_base *> need_param;

			if (*arg == '-')
			{
				++arg;

				if (*arg == '-') // double --, start of new argument
				{
					++arg;

					if (*arg == 0)
						break;

					std::string_view s_arg(arg);
					std::string_view::size_type p = s_arg.find('=');
					std::string_view p_arg;

					if (p != std::string_view::npos)
					{
						p_arg = s_arg.substr(p + 1);
						s_arg = s_arg.substr(0, p);
					}

					auto option = m_impl->get_option(s_arg);
					if (option == nullptr)
					{
						if (m_ignore_unknown_options)
							continue;

						throw unknown_option_error("Unkown option "s + std::string{ s_arg });
					}

					if (option->is_flag())
					{
						if (not p_arg.empty())
							throw config_option_error("Option " + option->m_name + " does not accept an argument");

						++option->m_seen;
						continue;
					}

					if (p_arg.empty())
					{
						if (i + 1 == argc)
							throw config_option_error("Missing argument for option " + option->m_name);
						
						++i;
						p_arg = argv[i];
					}

					++option->m_seen;
					option->set_value(p_arg);
					continue;
				}

				if (*arg != 0)
				{
					while (*arg != 0)
					{
						auto opt = m_impl->get_option(*arg++);
						if (opt)
						{
							++opt->m_seen;
							continue;
						}

						if (m_ignore_unknown_options)
							continue;

						throw unknown_option_error("Unkown option "s + *arg);
					}

					continue;
				}
			}
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
			throw no_param_error("The option " + name + " has no paramter");

		auto v_opt = dynamic_cast<option<T> *>(opt);
		if (v_opt == nullptr)
			throw invalid_param_type_error("The option " + name + " has a different type than requested");

		if (not v_opt->m_value)
			throw no_param_error("The option " + name + " has no paramter");

		return *v_opt->m_value;
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
void config::option<int>::set_value(std::string_view argument)
{
	int value;
	auto r = std::from_chars(argument.begin(), argument.end(), value);
	if (r.ec != std::errc())
		throw invalid_argument_error("Invalid argument for option " + m_name);
	m_value = value;
}

template <>
void config::option<std::filesystem::path>::set_value(std::string_view argument)
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
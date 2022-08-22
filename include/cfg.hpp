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

#include <memory>
#include <optional>

namespace cfg
{

class unknown_option_error : public std::runtime_error
{
  public:
	unknown_option_error(const std::string &err)
		: runtime_error(err) {}
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
			if (m_name.length() > 2 and m_name[m_name.length() - 2] == ',')
			{
				m_short_name = m_name.back();
				m_name.erase(m_name.end() - 2, m_name.end());
			}
		}

		virtual ~option_base() = default;
		virtual option_base *clone() = 0;
		virtual bool is_flag() const = 0;
	};

	template <typename T>
	struct option : public option_base
	{
		std::optional<T> m_default;

		option(const option &rhs) = default;

		option(std::string_view name)
			: option_base(name)
		{
		}

		option(std::string_view name, const T &v)
			: option_base(name)
			, m_default(v)
		{
		}

		option_base *clone() override
		{
			return new option(*this);
		}

		bool is_flag() const override { return false; }
	};

	template <typename... Options>
	void init(Options... options)
	{
		m_impl.reset(new config_impl(std::forward<Options>(options)... ));
	}

	void parse(int argc, const char *const argv[])
	{
		using namespace std::literals;

		for (int i = 1; i < argc; ++i)
		{
			const char *arg = argv[i];

			if (*arg == '-')
			{
				++arg;
				
				if (*arg == '-')	// double --, start of new argument
				{
					++arg;

					if (*arg == 0)
						break;
					
					auto option = m_impl->get_option(arg);
					if (option == nullptr)
					{
						if (m_ignore_unknown_options)
							continue;
						
						throw unknown_option_error("Unkown option "s + arg);
					}
						
					++option->m_seen;

					if (option->is_flag())
						continue;
				}
			}
			


			if (std::strcmp(arg, "--") == 0)
				break;

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

		std::vector<option_base*> m_options;
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

template<typename T = void>
auto make_option(std::string_view name)
{
	return config::option<T>(name);
}

template<typename T>
auto make_option(std::string_view name, const T &v)
{
	return config::option<T>(name, v);
}

} // namespace cfg
/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Maarten L. Hekkelman
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

#include "mcfp/error.hpp"
#include "mcfp/text.hpp"
#include "mcfp/utilities.hpp"

#include "mcfp/detail/options.hpp"

// --------------------------------------------------------------------

namespace mcfp::detail
{

class section
{
  public:
	using option_base = detail::option_base;

	template <typename... Options>
	section(std::string_view name, Options... options)
		: m_name(name)
		, m_impl(new config_impl<Options...>(std::forward<Options>(options)...))
	{
	}

	const std::string &name() const noexcept { return m_name; }

	void write(std::ostream &os, size_t indent, size_t output_width) const
	{
		if (m_name.empty())
			os << '\n';
		else
			os << "section " << m_name << "\n\n";

		m_impl->write(os, indent, output_width);
	}

	option_base *get_option(std::string_view name) const
	{
		return m_impl->get_option(name);
	}

	option_base *get_option(char short_name) const
	{
		return m_impl->get_option(short_name);
	}

	size_t get_option_width() const
	{
		return m_impl->get_option_width();
	}

  private:
	// --------------------------------------------------------------------

	struct config_impl_base
	{
		virtual ~config_impl_base() = default;

		virtual option_base *get_option(std::string_view name) = 0;
		virtual option_base *get_option(char short_name) = 0;

		virtual size_t get_option_width() const = 0;
		virtual void write(std::ostream &os, size_t wrap_width, size_t output_width) const = 0;

		virtual config_impl_base *next() const noexcept { return nullptr; }
	};

	template <typename... Options>
	struct config_impl : public config_impl_base
	{
		static constexpr size_t N = sizeof...(Options);

		config_impl(Options... options)
			: m_options(std::forward<Options>(options)...)
		{
		}

		option_base *get_option(std::string_view name) override
		{
			return get_option<0>(name);
		}

		template <size_t Ix>
		option_base *get_option([[maybe_unused]] std::string_view name)
		{
			if constexpr (Ix == N)
				return nullptr;
			else
			{
				option_base &opt = std::get<Ix>(m_options);
				return (opt.m_name == name) ? &opt : get_option<Ix + 1>(name);
			}
		}

		option_base *get_option(char short_name) override
		{
			return get_option<0>(short_name);
		}

		template <size_t Ix>
		option_base *get_option([[maybe_unused]] char short_name)
		{
			if constexpr (Ix == N)
				return nullptr;
			else
			{
				option_base &opt = std::get<Ix>(m_options);
				return (opt.m_short_name == short_name) ? &opt : get_option<Ix + 1>(short_name);
			}
		}

		virtual size_t get_option_width() const override
		{
			return std::apply([](Options const &...opts)
				{
				size_t width = 0;
				((width = std::max(width, opts.width())), ...);
				return width; }, m_options);
		}

		virtual void write(std::ostream &os, size_t wrap_width, size_t output_width) const override
		{
			std::apply([&os, wrap_width, output_width](Options const &...opts)
				{ (opts.write(os, wrap_width, output_width), ...); }, m_options);
		}

		std::tuple<Options...> m_options;
	};

	template <typename... Options>
	struct lib_config_impl : public config_impl<Options...>
	{
		lib_config_impl(std::string_view lib_name, Options... options)
			: config_impl<Options...>(std::forward<Options>(options)...)
			, m_lib_name(lib_name)
		{
		}

		config_impl_base *next() const noexcept override { return m_next; }

		std::string m_lib_name;
		config_impl_base *m_next = nullptr;
	};

	std::string m_name;
	std::unique_ptr<config_impl_base> m_impl;
};

} // namespace mcfp::detail

// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if USE_MODULE_STD
import std;
#else
# include <charconv>
# include <filesystem>
# include <iostream>
# include <optional>
# include <string>
# include <system_error>
# include <utility>
# include <vector>

#endif

#if __has_include(<sys/ioctl.h>)
// # include <fcntl.h>
# include <sys/ioctl.h>
# include <unistd.h>
#elif defined(_WIN32)
# include <Windows.h>
# include <cstdio>
# include <io.h>
#endif

export module mcfp;

#define MCFP_EXPORT export
#define MCFP_INLINE

#if defined(_WIN32) && defined(MCFP_SHARED_BUILD)
# define MCFP_API __declspec(dllexport)
#else
# define MCFP_API
#endif

// clang-format off
#include "error.hpp"
#include "charconv.hpp"
#include "options.hpp"
#include "sections.hpp"
#include "text.hpp"
#include "mcfp.hpp"
// clang-format on
// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <algorithm>
#include <cassert>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include(<sys/ioctl.h>)
# include <fcntl.h>
# include <sys/ioctl.h>
# include <unistd.h>
#elif defined(_WIN32)
# include <Windows.h>
# include <cstdio>
# include <io.h>
#endif

export module mcfp;

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

export import :charconv;
export import :error;
export import :options;
export import :sections;
export import :text;

#include "mcfp.hpp"
// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

import std;

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

#define IN_MODULE_INTERFACE
#define MCFP_EXPORT export
#define MCFP_INLINE

#include "error.hpp"
#include "charconv.hpp"
#include "options.hpp"
#include "sections.hpp"
#include "text.hpp"
#include "mcfp.hpp"

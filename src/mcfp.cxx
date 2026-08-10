// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cassert>

#if USE_MODULE_STD
import std;
#else
#include <algorithm>
#include <charconv>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#endif

#if __has_include(<sys/ioctl.h>)
// #  include <fcntl.h>
# include <sys/ioctl.h>
# include <unistd.h>
#elif defined(_WIN32)
# include <Windows.h>
# include <cstdio>
# include <io.h>
#endif

module mcfp;

#if USE_MODULE_STD
import std;
#endif

#if defined(USE_FAST_FLOAT)
#include "charconv-fast_float.cpp"
#endif
#include "options.cpp"
#include "text.cpp"
#include "mcfp.cpp"

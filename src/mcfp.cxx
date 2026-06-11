// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cassert>

# if __has_include(<sys/ioctl.h>)
// #  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <unistd.h>
# elif defined(_WIN32)
#  include <Windows.h>
#  include <cstdio>
#  include <io.h>
#endif

module mcfp;

import std;

#include "mcfp.cpp"

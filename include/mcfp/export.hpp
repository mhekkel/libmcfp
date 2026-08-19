// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifndef MCFP_EXPORT
# define MCFP_EXPORT
#endif

#ifndef MCFP_INLINE
# define MCFP_INLINE inline
#endif

#ifndef MCFP_API
# if defined(_WIN32) && defined(MCFP_SHARED_BUILD)
#  define MCFP_API __declspec(dllexport)
# else
#  define MCFP_API
# endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
/// \file       tuklib_common.h
/// \brief      Common definitions for tuklib modules
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TUKLIB_COMMON_H
#define TUKLIB_COMMON_H

// The config file may be replaced by a package-specific file.
// It should include at least stddef.h, inttypes.h, and limits.h.
#include "tuklib_config.h"

#define TUKLIB_CAT_X(a, b) a ## b
#define TUKLIB_CAT(a, b) TUKLIB_CAT_X(a, b)

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#	define TUKLIB_GNUC_REQ(major, minor) \
		((__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)) \
			|| __GNUC__ > (major))
#else
#	define TUKLIB_GNUC_REQ(major, minor) 0
#endif

#endif

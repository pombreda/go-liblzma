/**
 * \file        lzma.h
 * \brief       The public API of liblzma
 *
 * liblzma is a LZMA compression library with a zlib-like API.
 * liblzma is based on LZMA SDK found from http://7-zip.org/sdk.html.
 *
 * \author      Copyright (C) 1999-2006 Igor Pavlov
 * \author      Copyright (C) 2007 Lasse Collin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef LZMA_H
#define LZMA_H

/*****************************
 * Required standard headers *
 *****************************/

/**
 * liblzma API headers need some standard types and macros. To allow
 * including lzma.h without requiring the application to include other
 * headers first, lzma.h includes the required standard headers unless
 * they already seem to be included.
 *
 * Here's what types and macros are needed and from which headers:
 *  - stddef.h: size_t, NULL
 *  - stdint.h: uint8_t, uint32_t, uint64_t, UINT32_C(n), uint64_C(n),
 *    UINT32_MAX, UINT64_MAX
 *
 * However, inttypes.h is a little more portable than stdint.h, although
 * inttypes.h declares some unneeded things compared to plain stdint.h.
 *
 * The hacks below aren't perfect, specifically they assume that inttypes.h
 * exists and that it typedefs at least uint8_t, uint32_t, and uint64_t,
 * and that unsigned int is 32-bit. If your application already takes care
 * of setting up all the types properly (for example by using gnulib's
 * stdint.h or inttypes.h), feel free to define LZMA_MANUAL_HEADERS before
 * including lzma.h.
 *
 * Some could argue that liblzma API should provide all the required types,
 * for example lzma_uint64, LZMA_UINT64_C(n), and LZMA_UINT64_MAX. This was
 * seen unnecessary mess, since most systems already provide all the necessary
 * types and macros in the standard headers.
 *
 * Note that liblzma API still has lzma_bool, because using stdbool.h would
 * break C89 and C++ programs on many systems.
 */

/* stddef.h even in C++ so that we get size_t in global namespace. */
#include <stddef.h>

#if !defined(UINT32_C) || !defined(UINT64_C) \
		|| !defined(UINT32_MAX) || !defined(UINT64_MAX)
#	ifdef __cplusplus
		/*
		 * C99 sections 7.18.2 and 7.18.4 specify that in C++
		 * implementations define the limit and constant macros only
		 * if specifically requested. Note that if you want the
		 * format macros too, you need to define __STDC_FORMAT_MACROS
		 * before including lzma.h, since re-including inttypes.h
		 * with __STDC_FORMAT_MACROS defined doesn't necessarily work.
		 */
#		ifndef __STDC_LIMIT_MACROS
#			define __STDC_LIMIT_MACROS 1
#		endif
#		ifndef __STDC_CONSTANT_MACROS
#			define __STDC_CONSTANT_MACROS 1
#		endif
#	endif

#	include <inttypes.h>

	/*
	 * Some old systems have only the typedefs in inttypes.h, and lack
	 * all the macros. For those systems, we need a few more hacks.
	 * We assume that unsigned int is 32-bit and unsigned long is either
	 * 32-bit or 64-bit. If these hacks aren't enough, the application
	 * has to use setup the types manually before including lzma.h.
	 */
#	ifndef UINT32_C
#		define UINT32_C(n) n # U
#	endif

#	ifndef UINT64_C
		/* Get ULONG_MAX. */
#		ifndef __cplusplus
#			include <limits.h>
#		else
#			include <climits>
#		endif
#		if ULONG_MAX == 4294967295UL
#			define UINT64_C(n) n ## ULL
#		else
#			define UINT64_C(n) n ## UL
#		endif
#	endif

#	ifndef UINT32_MAX
#		define UINT32_MAX (UINT32_C(4294967295))
#	endif

#	ifndef UINT64_MAX
#		define UINT64_MAX (UINT64_C(18446744073709551615))
#	endif
#endif


/******************
 * GCC extensions *
 ******************/

/*
 * GCC extensions are used conditionally in the public API. It doesn't
 * break anything if these are sometimes enabled and sometimes not, only
 * affects warnings and optimizations.
 */
#if __GNUC__ >= 3
#	ifndef lzma_attribute
#		define lzma_attribute(attr) __attribute__(attr)
#	endif

#	ifndef lzma_restrict
#		define lzma_restrict __restrict__
#	endif

	/* warn_unused_result was added in GCC 3.4. */
#	ifndef lzma_attr_warn_unused_result
#		if __GNUC__ == 3 && __GNUC_MINOR__ < 4
#			define lzma_attr_warn_unused_result
#		endif
#	endif

#else
#	ifndef lzma_attribute
#		define lzma_attribute(attr)
#	endif

#	ifndef lzma_restrict
#		if __STDC_VERSION__ >= 199901L
#			define lzma_restrict restrict
#		else
#			define lzma_restrict
#		endif
#	endif

#	define lzma_attr_warn_unused_result
#endif


#ifndef lzma_attr_pure
#	define lzma_attr_pure lzma_attribute((__pure__))
#endif

#ifndef lzma_attr_const
#	define lzma_attr_const lzma_attribute((__const__))
#endif

#ifndef lzma_attr_warn_unused_result
#	define lzma_attr_warn_unused_result \
		lzma_attribute((__warn_unused_result__))
#endif


/**************
 * Subheaders *
 **************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Subheaders check that this is defined. It is to prevent including
 * them directly from applications.
 */
#define LZMA_H_INTERNAL 1

/* Basic features */
#include "lzma/version.h"
#include "lzma/init.h"
#include "lzma/base.h"
#include "lzma/vli.h"
#include "lzma/check.h"

/* Filters */
#include "lzma/filter.h"
#include "lzma/subblock.h"
#include "lzma/simple.h"
#include "lzma/delta.h"
#include "lzma/lzma.h"

/* Container formats */
#include "lzma/container.h"

/* Advanced features */
#include "lzma/alignment.h" /* FIXME */
#include "lzma/block.h"
#include "lzma/index.h"
#include "lzma/index_hash.h"
#include "lzma/stream_flags.h"
#include "lzma/memlimit.h"

/*
 * All subheaders included. Undefine LZMA_H_INTERNAL to prevent applications
 * re-including the subheaders.
 */
#undef LZMA_H_INTERNAL

#ifdef __cplusplus
}
#endif

#endif /* ifndef LZMA_H */

/* $Id$
 *
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef __sd_malloc_h
#define __sd_malloc_h

#include <stddef.h>
#include <stdlib.h>
#include "defs.h"

/**
 * @file malloc.h
 */

__SD_BEGIN_DECLS

typedef void (*sd_malloc_handler_t)();

SD_API sd_malloc_handler_t sd_malloc_set_handler(void (*a_handler)());

#ifndef __SD_DEBUG__

SD_API void *sd_malloc(size_t n);
SD_API void *sd_calloc(size_t n, size_t s);
SD_API void *sd_realloc(void *p, size_t n);
SD_API char *sd_strdup (const char *__str);

#else

#define sd_malloc	malloc
#define sd_calloc	calloc
#define sd_realloc	realloc
#define sd_strdup	strdup

#endif

__SD_END_DECLS

#endif

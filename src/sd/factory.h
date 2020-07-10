/* $Id$
 *
 * factory.h
 * 
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef __sd_factory_h
#define __sd_factory_h

/**
 * @file factory.h
 */

#include <stdio.h>
#include "defs.h"

__SD_BEGIN_DECLS

struct __sd_factory;
typedef struct __sd_factory sd_factory_t;

struct __sd_factory_ops
{
    void* (*fac_new)	(const char*);
    void  (*fac_delete)	(void*);
    void  (*fac_print)	(void*, FILE*);
};
typedef struct __sd_factory_ops sd_factory_ops_t;

SD_API sd_factory_t* sd_factory_new(const char* a_name, 
				    const sd_factory_ops_t* a_ops);
SD_API void	sd_factory_delete(sd_factory_t* a_this);
SD_API void*	sd_factory_get(sd_factory_t* a_this, const char* a_name);
SD_API void	sd_factory_destroy(sd_factory_t* a_this, void* a_pr);
SD_API void	sd_factory_print(const sd_factory_t* a_this, FILE* a_stream);
SD_API int	sd_factory_list(const sd_factory_t* a_this, void** a_items,
				int a_nitems);

__SD_END_DECLS

#endif

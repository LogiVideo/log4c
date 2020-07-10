/* $Id$
 *
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef __sd_error_h
#define __sd_error_h

#include <stdarg.h>
#include "defs.h"

SD_API int sd_debug(const char *fmt, ...);
SD_API int sd_error(const char *fmt, ...);

#endif /* __sd_error_h */

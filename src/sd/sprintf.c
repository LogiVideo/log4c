static const char version[] = "$Id$";

/* 
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "sprintf.h"
#include "malloc.h"
#include "sd_xplatform.h"

#ifndef va_copy
#pragma message("va_copy not defined for this platform.  The following implementation may not work correctly.")
#define va_copy(d, s)  d = s
#endif

/******************************************************************************/
SD_API char* sd_sprintf(const char* a_fmt, ...)
{
    char*	buffer;
    va_list	args;

    va_start(args, a_fmt);
    buffer = sd_vsprintf(a_fmt, args);
    va_end(args);

    return buffer;
}

/******************************************************************************/
SD_API char* sd_vsprintf(const char* a_fmt, va_list a_args)
{
    int		size	= 1024;
    char*	buffer  = (char*)sd_calloc(size, sizeof(char));
    int		n		= 0;

	while (buffer) {
	/* Make a copy of the va_list, so that we can reuse it on subsequent
	   iterations. */
	va_list ap_local;
	va_copy(ap_local, a_args);
	n = vsnprintf(buffer, size, a_fmt, ap_local);
	va_end(ap_local);
	
	/* If that worked, return */
	if (n > -1 && n < size)
	    return buffer;
	
	/* Else try again with more space. */
	if (n > -1)     /* ISO/IEC 9899:1999 */
	    size = n + 1;    
	else            /* twice the old size */
	    size *= 2;      
	
	buffer = (char*)sd_realloc(buffer, size);
    }

    return 0;
}

#if defined(__osf__)
#	ifndef snprintf
#		include "sprintf.osf.c"
#	endif
#endif


static const char version[] = "$Id$";

/*
 * layout.c
 *
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4c/layout.h>
#include <log4c/priority.h>
#include <sd/sprintf.h>
#include <sd/sd_xplatform.h>
#include <stdio.h>

/*******************************************************************************/
static const char* basic_format(
    const log4c_layout_t*	  	a_layout,
    const log4c_logging_event_t*	a_event)
{
    static char buffer[1024];

    int res = snprintf(buffer, sizeof(buffer), "%-8s %s - %s\n",
	     log4c_priority_to_string(a_event->evt_priority),
	     a_event->evt_category, a_event->evt_msg);
    
	/* If the output was truncated ellipsize the message and line-terminate it */
	if(res >= sizeof(buffer))
	{
		buffer[sizeof(buffer) - 5] =
		buffer[sizeof(buffer) - 4] =
		buffer[sizeof(buffer) - 3] = '.';
		buffer[sizeof(buffer) - 2] = '\n';
		buffer[sizeof(buffer) - 1] = '\0';
	}

    return buffer;
}

/*******************************************************************************/
const log4c_layout_type_t log4c_layout_type_basic = {
    "basic",
    basic_format,
};

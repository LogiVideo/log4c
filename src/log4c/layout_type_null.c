static const char version[] = "$Id$";

/*
 * layout.c
 *
 * Copyright 2008, Logitech. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4c/layout.h>
#include <log4c/priority.h>
#include <sd/sprintf.h>
#include <sd/sd_xplatform.h>
#include <stdio.h>

/*******************************************************************************/
static const char* null_format(const log4c_layout_t * a_layout,
    			       const log4c_logging_event_t * a_event)
{
	/* we simply copy the event into buffer; no additional formatting is done. */
    	static char buffer[1024];
    	snprintf(buffer, sizeof(buffer), "%s",a_event->evt_msg);
    	return buffer;
}

/*******************************************************************************/
const log4c_layout_type_t log4c_layout_type_null = {
    	"null",
    	null_format,
};


static const char version[] = "$Id$";

/*
 * layout.c
 *
 * Copyright 2001-2002, Cimai Technology SA (www.cimai.com). All rights reserved.
 * Copyright 2001-2002, Cedric Le Goater <legoater@cimai.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4c/layout.h>
#include <log4c/priority.h>
#include <sd/sprintf.h>
#include <stdio.h>

/*******************************************************************************/
static const char* basic_format(
    const log4c_layout_t*	  	a_layout,
    const log4c_logging_event_t*	a_event)
{
    static char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%-8s %s - %s\n",
	     log4c_priority_to_string(a_event->evt_priority),
	     a_event->evt_category, a_event->evt_msg);
    
    return buffer;
}

/*******************************************************************************/
const log4c_layout_type_t log4c_layout_type_basic = {
    name:	"basic",
    format:	basic_format,
};

log4c_layout_type_define(log4c_layout_type_basic);
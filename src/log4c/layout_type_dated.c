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
#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

/*******************************************************************************/
static const char* dated_format(
    const log4c_layout_t*  	a_layout,
    const log4c_logging_event_t*a_event)
{
    static char buffer[1024];
	int res;

#ifndef _WIN32
    struct timeval tv;
	struct tm tm;

	gettimeofday(&tv, 0);
	localtime_r(&tv.tv_sec, &tm);

	//gmtime_r(&a_event->evt_timestamp.tv_sec, &tm);
    res = snprintf(buffer, sizeof(buffer), "%04d%02d%02d %02d:%02d:%02d.%03ld %-8s %s- %s\r\n",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             a_event->evt_timestamp.tv_usec / 1000,
             log4c_priority_to_string(a_event->evt_priority),
             a_event->evt_category, a_event->evt_msg);
#else
	SYSTEMTIME stime = {0};
	FILETIME fileTimeSystem={0}, fileTimeLocal={0};

	GetSystemTimeAsFileTime(&fileTimeSystem);
	FileTimeToLocalFileTime(&fileTimeSystem, &fileTimeLocal);

	if ( FileTimeToSystemTime(&fileTimeLocal, &stime)){
    //if ( FileTimeToSystemTime(&a_event->evt_timestamp, &stime)){
    res = snprintf(buffer, sizeof(buffer), "%04d%02d%02d %02d:%02d:%02d.%03ld %-8s %s- %s\r\n",
             stime.wYear, stime.wMonth , stime.wDay,
             stime.wHour, stime.wMinute, stime.wSecond,
             stime.wMilliseconds,
             log4c_priority_to_string(a_event->evt_priority),
             a_event->evt_category, a_event->evt_msg);
        }
#endif

	/* If the output was truncated ellipsize the message and line-terminate it */
	if(res >= sizeof(buffer))
	{
		buffer[sizeof(buffer) - 6] =
		buffer[sizeof(buffer) - 5] =
		buffer[sizeof(buffer) - 4] = '.';
		buffer[sizeof(buffer) - 3] = '\r';
		buffer[sizeof(buffer) - 2] = '\n';
		buffer[sizeof(buffer) - 1] = '\0';
	}

    return buffer;
}

/*******************************************************************************/
const log4c_layout_type_t log4c_layout_type_dated = {
    "dated",
    dated_format,
};


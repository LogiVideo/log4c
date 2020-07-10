static const char version[] = "$Id$";

/*
 * layout_type_ISO8601.c
 *
 * Copyright 2008, Logitech (www.logitech.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <log4c/layout.h>
#include <log4c/priority.h>
#include <sd/sprintf.h>
#include <sd/sd_xplatform.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifndef _WIN32
#include <sys/time.h>
#endif


/*******************************************************************************/
static const char* ISO8601_format(const log4c_layout_t * a_layout,
    				  const log4c_logging_event_t * a_event)
{
	char* buffer = a_event->evt_buffer.buf_data;
	size_t bufferSize = a_event->evt_buffer.buf_size;

#ifndef _WIN32
	struct timeval tv;
	struct tm tm;
#else
	SYSTEMTIME stime={0};
	FILETIME fileTimeSystem={0}, fileTimeLocal={0};
#endif

	if (strcmp(a_event->evt_msg, ""))
	{
		int res;
#ifndef _WIN32
		gettimeofday(&tv, 0);
		localtime_r(&tv.tv_sec, &tm);

		res = snprintf(buffer, bufferSize,
			"%04d-%02d-%02dT%02d:%02d:%02d.%03d %-8s %-60s:   %s\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tv.tv_usec / 1000,
			log4c_priority_to_string(a_event->evt_priority),
			a_event->evt_category, a_event->evt_msg);
#else
		GetSystemTimeAsFileTime(&fileTimeSystem);
		FileTimeToLocalFileTime(&fileTimeSystem, &fileTimeLocal);
		FileTimeToSystemTime(&fileTimeLocal, &stime);

		/* Note: log4c is playing with fire by redefining snprintf to _snprintf on Windows.
		 *       Despite their similar names they don't actually do the same. In particular,
		 *       _snprintf does not always null-terminate the target buffer (notably if the return
		 *       value is '>= bufferSize'). A C99-compliant snprintf function, however, always
		 *       does.
		 *       In our case the check below makes sure that the target buffer is always null-
		 *       terminated if the output was truncated.
		 */
		res = snprintf(buffer, bufferSize,
			"%04d-%02d-%02dT%02d:%02d:%02d.%03ld %-8s %-60s:   %s\n",
			stime.wYear, stime.wMonth , stime.wDay,
			stime.wHour, stime.wMinute, stime.wSecond,
			stime.wMilliseconds,
			log4c_priority_to_string(a_event->evt_priority),
			a_event->evt_category, a_event->evt_msg);
#endif

		/* If the output was truncated ellipsize the message and line-terminate it */
		if(res >= bufferSize)
		{
			buffer[bufferSize - 5] =
			buffer[bufferSize - 4] =
			buffer[bufferSize - 3] = '.';
			buffer[bufferSize - 2] = '\n';
			buffer[bufferSize - 1] = '\0';
		}
	}
	else
	{
		snprintf(buffer, bufferSize, "\n");
	}

	return buffer;
}

/*******************************************************************************/
const log4c_layout_type_t log4c_layout_type_ISO8601 = {
    "ISO8601",
    ISO8601_format,
};

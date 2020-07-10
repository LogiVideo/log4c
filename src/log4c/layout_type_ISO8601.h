/* $Id$
 *
 * layout_type_ISO8601.h
 * 
 * Copyright 2008, Logitech (www.logitech.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef log4c_layout_type_ISO8601_h
#define log4c_layout_type_ISO8601_h

/**
 * @file layout_type_ISO8601.h
 *
 * @brief Implement a layout that uses standards based ISO 8601 date format
 * 	  and also logs the mac address of the machine in question to avoid
 * 	  ambiguity when reading and/or combining logfiles.
 *
 * The ISO8601 layout has the following conversion pattern: 
 * 
 * @c "%d %M %P %c - %m\n".
 *
 * Where 
 * @li @c "%d" is the date of the logging event in ISO 8601 format
 * @li @c "%M" is the network MAC address of the local computer.
 * @li @c "%P" is the priority of the logging event
 * @li @c "%c" is the category of the logging event
 * @li @c "%m" is the application supplied message associated with the
 * logging event
 *
 * If fetching the mac address fails, we print out a phony string.
 **/

#include <log4c/defs.h>
#include <log4c/layout.h>

__LOG4C_BEGIN_DECLS

extern const log4c_layout_type_t log4c_layout_type_ISO8601;

__LOG4C_END_DECLS

#endif

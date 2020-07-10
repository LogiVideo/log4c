/* $Id$
 *
 * layout_type_null.h
 * 
 * Copyright 2008, Logitech. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef log4c_layout_type_null_h
#define log4c_layout_type_null_h

/**
 * @file layout_type_null.h
 *
 * @brief Implements a null layout.  This layout assumes messages have already been
 * formatted elsewhere, and does absolutely nothing.  Nice if you want to 
 * receive messages from another log4c application, since they are already formatted.
 *
 **/

#include <log4c/defs.h>
#include <log4c/layout.h>

__LOG4C_BEGIN_DECLS

extern const log4c_layout_type_t log4c_layout_type_null;

__LOG4C_END_DECLS

#endif

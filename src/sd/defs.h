/* $Id$
 *
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef __sd_defs_h
#define __sd_defs_h

#ifdef  __cplusplus
# define __SD_BEGIN_DECLS  extern "C" {
# define __SD_END_DECLS    }
#else
# define __SD_BEGIN_DECLS
# define __SD_END_DECLS
#endif

#if (defined(_WIN32) || defined(_WIN64))
#ifdef SD_EXPORTS
# define SD_API extern
#else
# define SD_API extern
#endif
#else
# define SD_API extern
#endif

#endif

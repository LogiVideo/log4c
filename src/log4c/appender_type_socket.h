/* $Id$
 *
 * appender_type_socket.h
 * 
 * Copyright 2008, Logitech (www.logitech.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef log4c_appender_type_socket_h
#define log4c_appender_type_socket_h

/**
 * @file appender_type_socket.h
 *
 * @brief Log4c socket appender interface.
 *
 * The socket appender writes log information to a remote socket.
 * 
 * To use a socket appender, specify a type of "socket" in your log4c configuration
 * file:
 * 
 * <appender name="udp_socket" type="socket" layout="dated"/>
 * 
 * This will create a socket appender with the default destination (127.0.0.1) and
 * port (58231).  To specify destination and port, modify the XML element to look
 * like so:
 * 
 * <appender name="udp_socket" type="socket" layout="dated" dest="192.168.1.28" destport="29999"/>
 *
 * The above line will send a UDP stream to 192.168.1.28 on port 29999.
 **/

#include <log4c/defs.h>
#include <log4c/appender.h>

__LOG4C_BEGIN_DECLS

/**
 * Stream appender type definition.
 *
 * This should be used as a parameter to the log4c_appender_set_type()
 * routine to set the type of the appender.
 *
 **/
extern const log4c_appender_type_t log4c_appender_type_socket;

#define DEFAULT_DESTINATION_PORT "58231"
#define DEFAULT_DESTINATION "127.0.0.1"

typedef struct __socket_udata socket_udata_t; /* opaque */

/**
 * Get a new socket appender configuration object.
 * @return a new socket appender configuration object, otherwise NULL.
*/
LOG4C_API socket_udata_t* socket_make_udata(void);

/**
 * Set the logging destination in this socket appender configuration.
 * @param sock_data: the socket configuration object.
 * @param dest the destination to forward log messages to.
 * @return zero if successful, non-zero otherwise.
 */
LOG4C_API int socket_udata_set_dest(
                socket_udata_t *sock_data,
                char* dest);
/**
 * Set the logging destination port in this socket appender configuration.
 * @param sock_data the socket configuration object.
 * @param destport the port to which we forward log messages to.
 * @return zero if successful, non-zero otherwise.
 */
LOG4C_API int socket_udata_set_destport(
                socket_udata_t *sock_data,
                char* destport);

__LOG4C_END_DECLS

#endif

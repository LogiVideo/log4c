static const char version[] = "$Id$";

/*
 * appender_type_socket.c
 *
 * Copyright 2008, Logitech (www.logitech.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4c/appender.h>
#include <log4c/appender_type_socket.h>
#include <sd/malloc.h>

#ifndef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket(x) close(x)
#endif


struct __socket_udata {
	const char* dest;
	const char* destport;
	int sockfd;
	struct sockaddr_in sockaddr;
};

/*******************************************************************************/
static int socket_open(log4c_appender_t* this)
{
	int ra = 1;
	unsigned long block_flag = 1;

	socket_udata_t* sock = log4c_appender_get_udata(this); 
	sock->sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock->sockfd == INVALID_SOCKET)
	{
		return -1;
	}

#ifdef _WIN32
	ioctlsocket(sock->sockfd, FIONBIO, &block_flag );
#else
	block_flag = fcntl(sock->sockfd,F_GETFL,0);

	fcntl(sock->sockfd,F_SETFL,O_NONBLOCK|block_flag);
#endif

	if (setsockopt(sock->sockfd, SOL_SOCKET, SO_REUSEADDR,(char*)&ra, sizeof(ra)) == SOCKET_ERROR )
	{
        if(sock->sockfd != INVALID_SOCKET)
		{
			closesocket(sock->sockfd);
			sock->sockfd = INVALID_SOCKET;
		}
		return -1;
	}

	memset((void*)&sock->sockaddr, 0, sizeof(struct sockaddr_in));
	sock->sockaddr.sin_family = AF_INET;
	sock->sockaddr.sin_port = htons(atoi(sock->destport));
	sock->sockaddr.sin_addr.s_addr = inet_addr(sock->dest);

	return 0;
}

/*******************************************************************************/
static int socket_append(log4c_appender_t* this, const log4c_logging_event_t* a_event)
{
	socket_udata_t* sock = log4c_appender_get_udata(this); 

	sendto(sock->sockfd, a_event->evt_rendered_msg, strlen(a_event->evt_rendered_msg), 0, (struct sockaddr *)&sock->sockaddr, sizeof(sock->sockaddr));

	return 0;
}

/*******************************************************************************/
static int socket_close(log4c_appender_t* this)
{
	socket_udata_t* sock = log4c_appender_get_udata(this); 

	/* close out our socket */	
	if(sock->sockfd != INVALID_SOCKET)
	{
		closesocket(sock->sockfd);
		sock->sockfd = INVALID_SOCKET;
	}

	return 0;
}

/* create a new instance of our socket_udata_t structure */
LOG4C_API socket_udata_t* socket_make_udata(void)
{
	socket_udata_t* sup = NULL;
	sup = (socket_udata_t*)sd_calloc(1, sizeof(socket_udata_t));
	return sup;
}

/*
 * Set the logging destination in this socket appender configuration.
 * return zero if successful, non-zero otherwise.
 */
LOG4C_API int socket_udata_set_dest(socket_udata_t* sock_data, char* dest)
{
	sock_data->dest = strdup( dest );
	return 0;
}
/*
 * Set the logging destination port in this socket appender configuration.
 * return zero if successful, non-zero otherwise.
 */
LOG4C_API int socket_udata_set_destport(
                socket_udata_t *sock_data,
                char* destport)
{
	sock_data->destport = strdup(destport);
	return 0;
}

/*******************************************************************************/
const log4c_appender_type_t log4c_appender_type_socket = {
    "socket",
    socket_open,
    socket_append,
    socket_close,
};
/* -------------------------------------------------------- *
 * socket.h
 * ========
 * 
 * Summary 
 * -------
 * Socket abstractions.
 * NOTE: May retire these in the future. 
 * 
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ----------
 * - 
 * -------------------------------------------------------- */
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "../vendor/zwalker.h"
#include "../vendor/ztable.h"

#ifndef SOCKET_H
#define SOCKET_H

#define populate_tcp_socket(sock,port) \
	populate_socket(sock, IPPROTO_TCP, SOCK_STREAM, port)

#define populate_udp_socket(sock,port) \
	populate_socket(sock, IPPROTO_UDP, SOCK_DGRAM, port)

struct sockAbstr {
	int addrsize;
	int buffersize;
	int opened;
	int backlog;
	int waittime;
	int protocol;
	int socktype;
	int fd;
	int iptype; //ipv4 or v6
	int reuse;
	int family;
	int *port;
	struct sockaddr_in *sin;	
	struct sockaddr addrinfo; 
	socklen_t addrlen;
	char iip[ 16 ];	
};

int read_from_socket ( int fd, uint8_t **b, void (*readMore)(int *, void *) );
void whatsockerr( int e ) ;
void print_socket ( struct sockAbstr * );
struct sockAbstr * populate_socket ( struct sockAbstr *, int, int, int *);
struct sockAbstr * open_connecting_socket ( struct sockAbstr *, char *, int );
struct sockAbstr * close_connecting_socket ( struct sockAbstr *, char *, int );
struct sockAbstr * open_listening_socket ( struct sockAbstr *, char *, int );
struct sockAbstr * close_listening_socket ( struct sockAbstr *, char *, int );
struct sockAbstr * accept_listening_socket ( struct sockAbstr *, int *fd, char *, int );
struct sockAbstr * set_nonblock_on_socket ( struct sockAbstr *, char *, int );
struct sockAbstr * set_timeout_on_socket ( struct sockAbstr *, int );
int get_iip_of_socket( struct sockAbstr *a );

#endif

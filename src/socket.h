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
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * Changelog 
 * ----------
 * 
 * -------------------------------------------------------- */
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"

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

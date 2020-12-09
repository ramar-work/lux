/* -------------------------------------------------------- *
 * server.c
 * ========
 * 
 * Summary 
 * -------
 * Header for server functions for Hypno's server
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
 * 
 * Changelog 
 * ----------
 * 
 * -------------------------------------------------------- */
#include "socket.h"
#include "config.h"
#include "../vendor/zhttp.h"

#ifndef SERVER_H
#define SERVER_H

#define RD_EAGAIN 88
#define WR_EAGAIN 89
#define AC_EAGAIN 21
#define AC_EMFILE 31
#define AC_EEINTR 41

#if 1
	//#define CAFILE   "/etc/ssl/certs/ca-certificates.crt"
	#define FPATH "tlshelp/"
	#define KEYFILE  FPATH "x509-server-key.pem"
	#define CERTFILE FPATH "x509-server.pem"
	#define CAFILE   FPATH "x509-ca.pem"
	#define CRLFILE  FPATH "crl.pem"
#else
	#define FPATH "tlshelp/collinshosting-final/"
	#define KEYFILE  FPATH "x509-collinshosting-key.pem"
	#define CERTFILE FPATH "x509-collinshosting-server.pem"
	#define CAFILE   FPATH "collinshosting_com.ca-bundle"
	#define CRLFILE  FPATH "crl.pem"
#endif

struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, struct config *, struct host * );
};

struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void (*init)( void ** );
	void (*free)( void ** );
	int (*pre)( int, struct config *, void ** );
	int (*post)( int, struct config *, void ** ); 
	struct filter *filters;
	char *config;
	void *data;
}; 

struct model {
	int (*exec)( int, void * );
	int (*stop)( int * );
	void *data;
};


//Added 12-08-20, track per connection data
struct cdata {
	int count;  //How many times until we hit the keep-alive mark?
	int status;  //What is the status of the request? (perhaps only go up if complete?)
	const void *global;  //Global READ-ONLY userdata
	void *scope;    //Locally scoped userdata (only good for the life of the request)
	char *ipv4;  //The IPv4 address of the incoming request (in string format)
	char *ipv6;  //The IPv6 address of the incoming request (in string format)
	int flags; //Flags for the connection in question
};

int srv_response ( int, struct senderrecvr *, struct cdata * );

int srv_setsocketoptions ( int fd );

int srv_writelog ( int fd, struct sockAbstr *su );
#endif 

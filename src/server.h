/* -------------------------------------------------------- *
 * server.h
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
 * CHANGELOG 
 * ----------
 * 
 * -------------------------------------------------------- */
#include "luabind.h"
#include "socket.h"
#include "util.h"
#include "lconfig.h"
#include "../vendor/zhttp.h"
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "../vendor/zhttp.h"

#ifndef SERVER_H
#define SERVER_H

struct cdata;
struct filter;
struct senderrecvr;

//Added 12-08-20, track per connection data
struct cdata {
	int count;  //How many times until we hit the keep-alive mark?
	int status;  //What is the status of the request? (perhaps only go up if complete?)
	struct senderrecvr *ctx;  //Access the context through here now
	struct sconfig *config;  //Server config
	struct lconfig *hconfig;  //Host config
	char *ipv4;  //The IPv4 address of the incoming request (in string format)
	char *ipv6;  //The IPv6 address of the incoming request (in string format)
	int flags; //Flags for the connection in question
};


//Filter for each request interpreter 
struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, struct cdata * );
};


//Each new request works in its own environment.
struct senderrecvr { 
	const int (*read)( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );
	const int (*write)( int, struct HTTPBody *, struct HTTPBody *, struct cdata * ); 
	void (*init)( void ** );
	void (*free)( void ** );
	const int (*pre)( int, struct cdata *, void ** );
	const int (*post)( int, struct cdata *, void ** ); 
	const struct filter *filters;
	const char *config;
	void *data;
}; 

int srv_response ( int, struct cdata * );
#endif 

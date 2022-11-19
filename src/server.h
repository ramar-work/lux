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
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 * 
 * CHANGELOG 
 * ----------
 * - 
 * -------------------------------------------------------- */
#include <zhttp.h>
#include <zwalker.h>
#include <ztable.h>
#include "lua.h"
#include "socket.h"
#include "util.h"
#include "configs.h"

#ifndef SERVER_H
#define SERVER_H

struct cdata;
struct filter;
struct senderrecvr;

//Added 12-08-20, track per connection data
struct cdata {
	int count;  //How many times until we hit the keep-alive mark?
	int status;  //What is the status of the request?
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
	const int (*filter)( int, zhttp_t *, zhttp_t *, struct cdata * );
};


//Each new request works in its own environment.
struct senderrecvr { 
	const int (*read)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*write)( int, zhttp_t *, zhttp_t *, struct cdata * ); 
	void (*init)( void ** );
	void (*free)( void ** );
	const int (*pre)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*post)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const struct filter *filters;
	const char *config;
	void *data;
}; 

int srv_response ( int, struct cdata * );
#endif 

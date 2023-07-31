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
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../lua.h"
#include "../util.h"
#include "../configs.h"

#ifndef SERVER_H
#define SERVER_H

#define SERVER_ERROR_BUFLEN 1024

#define SERVER_EBUFLEN 128 
#define CONNECTION_EBUFLEN 128 

#if 0
struct cdata;
//struct filter;
//struct senderrecvr;


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
#endif




// Individual connections need to pass their own stuff around.
typedef struct conn_t {

	// Keep track of how many times we've sent (or tried to send) 
	// data over this file
	int count;

	// Did the request fail or not?
	int status;

	// Keep a reference to the server's configuration
	struct sconfig *config;

	// Keep a reference to the host configuration
	struct lconfig *hconfig;

	// Global reference to make for less arguments
	// struct senderrecvr *ctx;	

	// The inner data shuttled between actions per connection 
	void *data;

	// Connection start
	struct timespec start;

	// Connection end
	struct timespec end;

	// The file descriptor in use for the current connection
	int fd;

	// Request
	zhttp_t *req;

	// Response
	zhttp_t *res;

	// Error length
	int errlen;

	// Error buffer
	char err[ 128 ];

	// Keep buffer for ipv4 address
	char ipv4[ 32 ];
	
	// Keep buffer for ipv6 address
	char ipv6[ 128 ];

} conn_t;




#if 0
//Each new request works in its own environment.
struct senderrecvr { 
	const int (*read)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*write)( int, zhttp_t *, zhttp_t *, struct cdata * ); 
	int (*init)( void **, char *, int );
	void (*free)( void ** );
	const int (*pre)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*post)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const struct filter *filters;
	const char *config;
	void *data;
	char error[ SERVER_ERROR_BUFLEN ];
};
#endif


typedef struct server_t {

	// Config file name
	char *conffile;

	// Parent fd
	int fd;

	// Can evaluate the server config once and be done?
	struct sconfig *config;

	// The port we started on
	short unsigned int port;

	// The backlog value used for this particular process
	int backlog;

	// Server start
	struct timespec start;
	
	// Server end 
	struct timespec end;

	// Outer data that is used by the entire process
	void *data;

	// If these are shared, this has to be a function, which handles locking
	FILE *log_fd;

	FILE *access_fd;

	int *fdset;

	// All the loaded filters can go here
	const struct filter *filters;

	// This does make sense to be here...
	struct senderrecvr *ctx;

	// 
	char err[ 128 ];

	int errlen;

} server_t;


// "Filter" output messages with this
struct filter {
	const char *name;
	//const int (*filter)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*filter)( const server_t *, conn_t * );
};



//Each new request works in its own environment.
struct senderrecvr { 
#if 0
	const int (*read)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*write)( int, zhttp_t *, zhttp_t *, struct cdata * ); 
	int (*init)( void **, char *, int );
	void (*free)( void ** );
	const int (*pre)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*post)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const struct filter *filters;
	const char *config;
	void *data;
	char error[ SERVER_ERROR_BUFLEN ];
#else
	const char *name;
	const int (*read)( server_t *, conn_t * );
	const int (*write)( server_t *, conn_t * ); 
	int (*init)( server_t * );
	void (*free)( server_t * );
	const int (*pre)( server_t *, conn_t * );
	const int (*post)( server_t *, conn_t * );
	//const char *config;
#endif
};


int srv_response ( server_t *, conn_t * );
#endif 

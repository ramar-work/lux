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
//filter_t;
//protocol_t;


//Added 12-08-20, track per connection data
struct cdata {
	int count;  //How many times until we hit the keep-alive mark?
	int status;  //What is the status of the request?
	protocol_t *ctx;  //Access the context through here now
	struct sconfig *config;  //Server config
	struct lconfig *hconfig;  //Host config
	char *ipv4;  //The IPv4 address of the incoming request (in string format)
	char *ipv6;  //The IPv6 address of the incoming request (in string format)
	int flags; //Flags for the connection in question
};
#endif


typedef enum status_t {
	CONN_INIT = 0,
	CONN_PRE,
	CONN_READ,
	CONN_PROC,
	CONN_WRITE,
	CONN_POST,
	CONN_DESTROY,
} status_t;


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

	// The inner data shuttled between actions per connection 
	void *data;

	// Are we reading, writing, etc (no more than 8 states)
	status_t stage;

	// Keep track of retries (128 is more than enough)
	int retry;

	// The file descriptor in use for the current connection
	int fd;

	// Request
	zhttp_t *req;

	// Response
	zhttp_t *res;

	// Error length
	int errlen;

	// Error buffer
	char err[ 256 ];

	// Keep buffer for ipv4 address
	char ipv4[ 32 ];
	
	// Keep buffer for ipv6 address
	char ipv6[ 128 ];

#ifdef DEBUG_H
	// Advanced timing data really should be a debug feature
	
	// Connection start
	struct timespec start;

	// Connection end
	struct timespec end;

	// Request processing time
	int request_time;

	// Response processing time
	int response_time;
#endif

} conn_t;




#if 0
//Each new request works in its own environment.
protocol_t { 
	const int (*read)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*write)( int, zhttp_t *, zhttp_t *, struct cdata * ); 
	int (*init)( void **, char *, int );
	void (*free)( void ** );
	const int (*pre)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*post)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const filter_t *filters;
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
	const struct filter_t *filters;

	// This does make sense to be here...
	struct protocol_t *ctx;

	// A default timeout value for long-running connections
	int timeout;

	// 
	char err[ 128 ];

	int errlen;

	// Catch an interrupt
	int interrupt;

	// Client max simultaneous
	short int max_per;

#ifdef DEBUG_H
	int tapout;
#endif

} server_t;


// "Filter" output messages with this
typedef struct filter_t {
	const char *name;
	const int (*filter)( const server_t *, conn_t * );
} filter_t;



//Each new request works in its own environment.
typedef struct protocol_t {
#if 0
	const int (*read)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*write)( int, zhttp_t *, zhttp_t *, struct cdata * ); 
	int (*init)( void **, char *, int );
	void (*free)( void ** );
	const int (*pre)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*post)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const filter_t *filters;
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
} protocol_t;


int srv_response ( server_t *, conn_t * );
#endif 

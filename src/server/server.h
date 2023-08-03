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



// Keep track of what's going on once hte connection has been made
typedef enum connstage_t {
	CONN_DORMANT = 0,
	CONN_INIT,
	CONN_PRE,
	CONN_READ,
	CONN_PROC,
	CONN_WRITE,
	CONN_POST,
	CONN_DESTROY,
} connstage_t;



// Keep track of a connection as it comes in
typedef enum connstatus_t {
	CONNSTAT_AVAILABLE = 0,
	CONNSTAT_ACTIVE,
	CONNSTAT_INACTIVE
} connstatus_t;



// All the properties we'll need for a server
typedef struct server_t {

	// Config file name
	char *conffile;

	// Parent fd
	int fd;

	// Most appropriate for threaded models 
	int *fdset;

	// A log file
	FILE *log_fd;

	// An access log file
	FILE *access_fd;

	// Can evaluate the server config once and be done?
	struct sconfig *config;

	// The port we started on
	unsigned short int port;

	// The backlog value used for this particular process
	unsigned short int backlog;

	// A default timeout value for long-running connections
	unsigned short int timeout;

	// Client max simultaneous
	unsigned short int max_per;

	// Server start
	struct timespec start;
	
	// Server end 
	struct timespec end;

	// Outer data that is used by the entire process
	void *data;

	// All the loaded filters can go here
	const struct filter_t *filters;

	// This does make sense to be here...
	struct protocol_t *ctx;

	// Catch an interrupt
	unsigned short int interrupt;

	// Write timeout
	unsigned short int wtimeout;

	// Read timeout
	unsigned short int rtimeout;

	// Total (?) timeout
	unsigned short int ttimeout;

	// Handshake timeout
	unsigned short int tls_handshake_timeout;

	// Total conncount
	int conncount;

	// 
	char err[ 128 ];

#ifdef DEBUG_H
	int tapout;
#endif

} server_t;




// Individual connections need to pass their own stuff around.
typedef struct conn_t {

#if 1
	// Keep track of how many times we've sent (or tried to send) 
	// data over this file
	int count;

	// Keep track of retries (128 is more than enough)
	int retry;

	// The status of the actual connection 
	connstatus_t running;

	// The stage of processing of said connection
	connstage_t stage;
#else
	// None of these will exceed 128, so we're safe to use 
	// chars or even bitfields
	char info;	
#endif

	// Easier if it's here..., but can track elsewhere
	pthread_t id;

#if 1
	// Reference the server configuration
	struct server_t *server;

	// Reference the host's configuration
	struct lconfig *config;
#else
	// Having this alone negates the need for the following
	struct server_t *server;

	// Keep a reference to the server's configuration
	struct sconfig *config;

	// Keep a reference to the host configuration
	struct lconfig *hconfig;
#endif

	// The inner data shuttled between actions per connection 
	void *data;

	// The file descriptor in use for the current connection
	int fd;

	// Request
	zhttp_t *req;

	// Response
	zhttp_t *res;

	// Error buffer
	char err[ 128 ];

	// Keep buffer for ipv4 address
	char ipv4[ 16 ];

	// Keep buffer for ipv6 address
	unsigned char ipv6[ 16 ];
	
	// Connection start
	struct timespec start;

	// Connection end
	struct timespec end;

} conn_t;



// "Filter" output messages with this
typedef struct filter_t {
	const char *name;
	const int (*filter)( const server_t *, conn_t * );
} filter_t;




//Each new request works in its own environment.
typedef struct protocol_t {
	const char *name;
	const int (*read)( server_t *, conn_t * );
	const int (*write)( server_t *, conn_t * ); 
	int (*init)( server_t * );
	void (*free)( server_t * );
	const int (*pre)( server_t *, conn_t * );
	const void (*post)( server_t *, conn_t * );
} protocol_t;


int srv_response ( server_t *, conn_t * );
#endif 

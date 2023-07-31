#include <stdio.h>
#include "../server/server.h"

#ifndef CTXCTX_H
#define CTXCTX_H


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
	const int (*read)( struct senderrecvr *, server_t *, conn_t * );
	const int (*write)( struct senderrecvr *, server_t *, conn_t * ); 
	int (*init)( void **, char *, int );
	void (*free)( void ** );
	const int (*pre)( struct senderrecvr *, server_t *, conn_t * );
	const int (*post)( struct senderrecvr *, server_t *, conn_t * );
	//const char *config;
#endif
	const struct filter *filters;
};


#if 0
typedef struct ctx_t {

	// Parent fd
	int fd;

	// Contextual functions
	struct senderrecvr *ctx;

	// Server start
	struct timespec start;
	
	// Server end 
	struct timespec end;

	// Filters
	// filters?
	
	// If these are shared, this has to be a function, which handles locking
	//FILE *log_fd;

	//FILE *access_fd;

	int *fdset;

} ctx_t;
#endif

#endif

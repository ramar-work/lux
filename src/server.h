#include "http.h"
#include "socket.h"

#ifndef SERVER_H
#define SERVER_H

#define RD_EAGAIN 88
#define WR_EAGAIN 89
#define AC_EAGAIN 21
#define AC_EMFILE 31
#define AC_EEINTR 41


struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * );
};

struct senderrecvr { 
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*accept)( struct sockAbstr *, int *, void *, char *, int );
	void (*free)( int, struct HTTPBody *, struct HTTPBody *, void * );
	void (*init)( void ** );
#if 0 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
#endif
	void *data;
}; 

struct model {
	int (*exec)( int, void * );
	int (*stop)( int * );
	void *data;
};

struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
};

#endif 

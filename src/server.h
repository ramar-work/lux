#include "http.h"

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

#if 0
struct rwctx {
	const char *name;
	void * (*create)();	
	int (*accept)( struct sockAbstr *, int *, char *, int );
	int (*read)( int, void *, uint8_t *, int  );	
	int (*write)( int, void *, uint8_t *, int  );
	void (*destroy)( void *ctx );	
	void *userdata;
};
#endif

struct senderrecvr { 
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*accept)( struct sockAbstr *, int *, char *, int );
#if 0
	int (*sread)( int, void *, uint8_t *, int  );	
	int (*swrite)( int, void *, uint8_t *, int  );
#endif
	void (*free)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
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

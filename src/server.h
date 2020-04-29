#include "http.h"

int srv_fork ( int fd );
int srv_thread ( int fd );
int srv_vanilla ( int fd );
int srv_test ( int fd );
int srv_dummy ( int *times );
int srv_inccount( int *times );
int srv_1kiter( int *times );
int h_read ( int, struct HTTPBody *, struct HTTPBody *, void * );
int h_proc ( int, struct HTTPBody *, struct HTTPBody *, void * );
int h_write( int, struct HTTPBody *, struct HTTPBody *, void * );
int t_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx );
int t_write( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx );


struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * );
};

struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void *readf;
	void *writef;
}; 

struct model {
	int (*exec)( int );
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

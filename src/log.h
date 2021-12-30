/*log.h*/
#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <string.h>
#include <time.h>

int f_open( char *name, void **d ) ;

int f_close( void *d ) ;

int f_write( void *d ) ;

char * f_handler() ;

int sqlite3_log_open ( char *name, void **d ) ;

int sqlite3_log_close( void *d ) ;

int sqlite3_log_write( void *dd ) ;

char * sqlite3_handler() ;

struct log {
	int (*open)( char *, void ** );
	int (*close)( void * );
	int (*write)( void * );
	char * (*handler)();
	void *data;
};

struct loginfo {
	char *ip;
	unsigned long reqid;
	struct timespec start;
	char *ident;
	char *id;
	char *method;
	char *proto;
	char *resource;
	char *ua;
	char *referer;
	int type;
	int date;
	int status;
	int size;
	struct timespec end;
};


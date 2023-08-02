/*log.h*/
#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifndef LOG_H
#define LOG_H

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


typedef struct loginfo_t {
#if 0
	unsigned long reqid;
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
#endif
	char *ip;
	struct timespec start;
	struct timespec end;
} loginfo_t;

#define time_current(u) \
	clock_gettime( CLOCK_REALTIME, u )

int time_format ( struct timespec *, char *, int );
int time_diff_sec ( struct timespec *, struct timespec * );
long time_diff_nsec ( struct timespec *, struct timespec * );

#endif

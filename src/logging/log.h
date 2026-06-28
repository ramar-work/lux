/**
 * log.h
 * -------
 *
 * Logging utilities for regular files and SQLite3
 *
 */
#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "../server/server.h"

#ifndef LOG_H
#define LOG_H

#define time_current(u) \
	clock_gettime( CLOCK_REALTIME, u )

int f_open( const char *, void **, char *, int ) ;

int f_close( void * ) ;

int f_write( void *, const server_t *, const conn_t * ) ;

int sqlite3_log_open ( const char *, void **, char *, int ) ;

int sqlite3_log_close( void * ) ;

int sqlite3_log_write( void *, const server_t *, const conn_t * ) ;

#endif

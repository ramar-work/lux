/* ------------------------------------------- * 
 * zdb.h
 * ========
 * 
 * Summary 
 * -------
 * Header file for functions for dealing with basic 
 * SQL database interactions.
 *
 * Usage
 * -----
 * gcc -ldl -llua -o zdb vendor/single.o zdb.c && ./config
 * 
 *
 * LICENSE
 * -------
 * Copyright 2020 - 2021 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include <strings.h>
#include <zwalker.h>

#ifndef ZDATABASE_H
#define ZDATABASE_H

#define ZDB_ERRBUF_LEN 256 

#define ZDB_MAX_COLUMNS 128

#define ZDB_MAX_BINDS 256

#define ZDB_ENABLE_EXTENSIONS

#define ZDB_ENABLE_SQLITE

#undef ZDB_ENABLE_MYSQL

#define zdbv_add_item(LIST,ELEMENT,SIZE,LEN) \
	zdb_add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

#ifdef ZDB_ENABLE_EXTENSIONS
 #include <ztable.h>
#endif 

typedef enum {
	ZDB_NONE
#ifdef ZDB_ENABLE_SQLITE
,	ZDB_SQLITE
#endif
#ifdef ZDB_ENABLE_MYSQL
,	ZDB_MYSQL
#endif
#ifdef ZDB_ENABLE_POSTGRESQL
	#error "PostgreSQL not yet supported."
,	ZDB_POSTGRESQL
#endif
#ifdef ZDB_ENABLE_ORACLE
	#error "Oracle not yet supported."
,	ZDB_ORACLE
#endif
} zdbb_t ;


//Structure used for something else...
typedef struct zdbv_t {
	const char *field;
	void *value;
	unsigned long len;
	zhType type;
} zdbv_t;


//Most actual databases need a ton of information to connect
typedef struct zdbconn_t {
	//POSTGRES just uses a string, and breaks it up for you
	//MYSQL uses a bunch of different functions
	char username[ 128 ];
	char password[ 128 ];
	char hostname[ 128 ];
	char dbname[ 128 ];
	char unixsock[ 128 ];
	int port;
	int timeout;
	//int **flags;
	void **options;
} zdbconn_t;


//The grandaddy of db structures...
typedef struct zdb_t {
	void * ptr;  // The pointer to whatever handle is in use...
	void * (*open)( struct zdb_t *, const char *, char *, int );
	void * (*close)( void **, char *, int );
	void * (*exec)( struct zdb_t *, const char *, zdbv_t ** );
	void * (*bind)( const char *str, void *p );
	zdbb_t type;  // The type of db driver 
	int rows; // Result set count
	int affected; // Result set count
	int error;
	short llen;
	const char **headers;
	zdbv_t **results;
	zdbconn_t conn;
	char err[ ZDB_ERRBUF_LEN ];
} zdb_t;


const char * zdb_str_error ( zdb_t *zdb );

zdb_t *zdb_open ( zdb_t *, const char *, zdbb_t );

void *zdb_close( zdb_t * );

void *zdb_exec( zdb_t *, const char *, zdbv_t ** );

zdbv_t ** zdb_bind ( zdb_t *, zdbv_t **, char *, void *, int, zhType );

zdbv_t ** zdb_make_bind_args ( zdbv_t **, void *, int , zhType );

void * zdb_add_item_to_list( void ***, void *, int, int * );

void zdbv_dump ( zdbv_t ** ) ;

void zdbv_free ( zdbv_t ** );

void zdb_free ( zdb_t * ) ;

#ifdef ZTABLE_H
zTable * zdb_to_ztable ( zdb_t *, const char * );
#endif

//By default, SQLite3 will always be enabled...
#ifdef ZDB_ENABLE_SQLITE
//Macros could really be improved here...
void *zdb_sqlite_open( zdb_t *, const char *, char *, int ) ;

void *zdb_sqlite_close( void **, char *, int );

void *zdb_sqlite_exec( zdb_t *, const char *, zdbv_t ** );
#endif

#ifdef ZDB_ENABLE_MYSQL
void *zdb_mysql_open( zdb_t *, const char *, char *, int ) ;

void *zdb_mysql_close( void **, char *, int );

void *zdb_mysql_exec( zdb_t *, const char *, zdbv_t ** );
#endif

#ifdef DEBUG_H
void zdbconn_print ( zdbconn_t * ) ;
#else
 #define zdbconn_print(...)
#endif

#define zdb_open_sqlite(a,b) 
#define zdb_open_mysql(a,b) 
#define zdb_open_postgres(a,b) 
#define zdb_open_oracle(a,b) 


#define db_query_file( ptr, file )

#define db_insert( ptr, file )

#define db_query_str( ptr, str )


#endif

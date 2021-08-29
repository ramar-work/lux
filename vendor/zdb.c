/* ------------------------------------------- * 
 * zdb.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with basic SQL database 
 * interactions.
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
#include "zdb.h"

#ifdef ZDB_ENABLE_SQLITE
 #include <sqlite3.h>
#endif

#ifdef ZDB_ENABLE_MYSQL
 #include <mysql/mysql.h>
#endif

#define AT_EOS(a) ( (a)->len == -1 && *(a)->field == eos )

enum zdb_error {
	ZDB_ERROR_NONE
, ZDB_ERROR_GENERIC
, ZDB_ERROR_UNSUPPORTED_BACKEND
, ZDB_ERROR_OPEN
, ZDB_ERROR_CLOSE
, ZDB_ERROR_NOQUERY
, ZDB_ERROR_PREPARE
, ZDB_ERROR_ALLOC
, ZDB_ERROR_BIND
, ZDB_ERROR_BINDPARAM
, ZDB_ERROR_BINDSTMT
, ZDB_ERROR_BINDMAX
, ZDB_ERROR_BINDNOVALUES
, ZDB_ERROR_COLUMNLENGTHEXCEEDMAX
, ZDB_ERROR_QUERY
, ZDB_ERROR_CONNSTRING
, ZDB_ERROR_STMTCLOSE
, ZDB_ERROR_STMTRESULT
};

static const char eos = 127;

static const char endset[] = { eos }; 

static const char *zdb_errors[] = {
	[ 0 ] = "No error." 
, [ ZDB_ERROR_GENERIC ] = "Generic error message..."
, [ ZDB_ERROR_UNSUPPORTED_BACKEND ] = "Unsupported backend."
, [ ZDB_ERROR_OPEN ] = "Error opening database."
, [ ZDB_ERROR_CLOSE ] = "Error closing database."
, [ ZDB_ERROR_NOQUERY ] = "No query/zero-length query specified."
, [ ZDB_ERROR_PREPARE ] = "%s"
, [ ZDB_ERROR_ALLOC ] = "Allocation error."
, [ ZDB_ERROR_BIND ] = "Could not bind parameter '%s'"
, [ ZDB_ERROR_BINDPARAM ] = "Could not locate bind parameter '%s'"
, [ ZDB_ERROR_BINDSTMT ] = "Problem with bind statement at %s"
, [ ZDB_ERROR_BINDMAX ] = "Attempting to bind too many values."
, [ ZDB_ERROR_BINDNOVALUES ] = "No values specified for binding."
, [ ZDB_ERROR_COLUMNLENGTHEXCEEDMAX ] = "Columns count is too long." 
, [ ZDB_ERROR_QUERY ] = "%s" 
, [ ZDB_ERROR_STMTCLOSE ] = "Could not close SQL statement."
, [ ZDB_ERROR_STMTRESULT ] = "SQL Statement retrieval result issue."
};



const char * zdb_str_error ( zdb_t *zdb ) {
	return ( !(*zdb->err) ) ? zdb_errors[ zdb->error ] : zdb->err;
};



static void * zdb_init( zdb_t *zdb, zdbb_t type ) {
	memset( zdb, 0, sizeof( zdb_t ));
	//TODO: This probably isn't necessary 
	memset( zdb->err, 0, ZDB_ERRBUF_LEN );
	zdb->type = type;

	if ( 0 ) ;
#ifdef ZDB_ENABLE_SQLITE
	else if ( zdb->type == ZDB_SQLITE ) {
		zdb->open = zdb_sqlite_open; 
		zdb->close = zdb_sqlite_close;
		zdb->exec = zdb_sqlite_exec;
	}
#endif
#ifdef ZDB_ENABLE_MYSQL
	else if ( zdb->type == ZDB_MYSQL ) {
		zdb->open = zdb_mysql_open; 
		zdb->close = zdb_mysql_close;
		zdb->exec = zdb_mysql_exec;
	}
#endif
	else {
		zdb->error = ZDB_ERROR_UNSUPPORTED_BACKEND;
		return NULL;
	}	

	return zdb;
}



//General db close function
zdb_t * zdb_open ( zdb_t *zdb, const char *dbname, zdbb_t type ) {

	//...
	if ( !zdb ) { 
		return NULL;
	}

	//Initialize
	if ( !zdb_init( zdb, type ) ) {
		return NULL;
	}

	//Open the pointer
	if ( !( zdb->ptr = zdb->open( zdb, dbname, zdb->err, ZDB_ERRBUF_LEN )) ) {
		zdb->error = ZDB_ERROR_OPEN;
		return NULL;
	}

	return zdb;
}



//General db close function
void * zdb_close ( zdb_t *zdb ) {
	if ( !zdb->close( &zdb->ptr, zdb->err, ZDB_ERRBUF_LEN ) ) {
		zdb->error = ZDB_ERROR_CLOSE;	
		return NULL;
	} 
	return zdb;
}




//Add to series
void * zdb_add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}



static zdbv_t * zdbv_init ( ) {
	zdbv_t *t = NULL;
	t = malloc( sizeof( zdbv_t ) );
	memset( t, 0, sizeof( zdbv_t ) );
	return t;
}


//Keep adding results...
static zdbv_t * zdbv_add_to_set ( zdbv_t ***list ) {
	return NULL;
}



char * zdb_typedump( zdbb_t type ) {
	//fprintf( stderr, "%s"
	static const char *list[] = {
	#ifdef ZDB_ENABLE_SQLITE
		[ZDB_SQLITE] = "SQLITE3",
	#endif
	#ifdef ZDB_ENABLE_MYSQL
		[ZDB_MYSQL] = "MYSQL",
	#endif
		NULL
	};
	return NULL;
}


void zdb_dump( zdb_t *zdb ) {
	fprintf( stderr, "zdb->error = %d\n", zdb->error );
	fprintf( stderr, "zdb->err = %s\n", zdb->err );
	fprintf( stderr, "zdb->ptr = %p\n", zdb->ptr );
	fprintf( stderr, "zdb->type = %d\n", zdb->type );
	fprintf( stderr, "zdb->rows = %d\n", zdb->rows );
	fprintf( stderr, "zdb->mapsize = %d\n", zdb->mapsize );
	fprintf( stderr, "zdb->affected = %d\n", zdb->affected );
	fprintf( stderr, "zdb->llen = %d\n", zdb->llen );
	fprintf( stderr, "%s\n", "zdb->conn" );
	zdbconn_print( &zdb->conn );
	fprintf( stderr, "%s\n", "zdb->headers" );
	for ( const char **h = zdb->headers; h && *h; h++ ) fprintf( stderr, "%s\n", *h );
	fprintf( stderr, "%s: %p\n", "zdb->results", zdb->results );
	//zdbv_dump( zdb->results );
}


void zdbv_dump ( zdbv_t **list ) {
	int a = 1, b = 0;
	for ( zdbv_t **r = list; r && *r; r++ ) {
		fprintf( stderr, "rec %d: '%s' => '", a++, (*r)->field );
		b = write( 2, (*r)->value, (*r)->len ); 
		b = write( 2, "'\n", 2 );
	} 
}



static const char * zdb_dupstr ( const char *a ) {
	const char *bb = malloc( strlen( a ) + 1 );
	memset( (void *)bb, 0, strlen( a ) + 1 );
	memcpy( (void *)bb, a, strlen( a ) );
	return bb;
}


//
void *zdb_exec( zdb_t *zdb, const char *query, zdbv_t **records ) {
	//Block bad queries...
	if ( !query || strlen( query ) < 3 ) {
		zdb->error = ZDB_ERROR_NOQUERY;
		return NULL;
	}

	//no rows, none affected, of course it will be null
	if ( !zdb->exec( zdb, query, records ) && zdb->error ) {
		return NULL;
	}

	//perhaps conversions can take place here..

	//If we don't get this far, there was a failure...
	return zdb; 
}



void zdbv_loop ( zdbv_t ** list ) {
	for ( zdbv_t **l = list; l && *l; l++ ) {
		fprintf( stderr, "%s: ", (*l)->field );
		int b = write( 2, (*l)->value, (*l)->len );
		fprintf( stderr, "\n" );
	}
}



//Only use these on generated lists
void zdb_free ( zdb_t * zdb ) {
	//Destroy results
	for ( zdbv_t **l = zdb->results; l && *l; l++ ) {
		free( (*l)->value ), free( *l );
	}
	free( zdb->results );

	//Destroy headers
	for ( const char **header = zdb->headers; header && *header; header++ ) {
		free( (void *)*header );
	}
	free( zdb->headers );
}





zdbv_t ** zdb_bind ( zdb_t *zdb, zdbv_t **recs, char *n, void *p, int len, zhType type ) {
	zdbv_t * zdbv = malloc( sizeof (zdbv_t) );
	if ( !zdbv || !memset( zdbv, 0, sizeof ( zdbv_t ) ) ) {
		return NULL;
	}
	zdbv->field = n;
	zdbv->value = p;
	zdbv->len = len;
	zdbv->type = type;
	zdbv_add_item( &recs, zdbv, zdbv_t *, (int *)&zdb->llen ); 
	return recs;
}



static int zdb_check_number ( char *ptr, int len ) {
	for ( ; *ptr; ptr++ ) {
		if ( !strchr( "0123456789", *ptr ) ) return 0;
	}
	return 1;	
}


//This is probably just simpler to do...
zdbconn_t * zdb_init_conn ( zdbconn_t *conn, const char *connstr, char *err, int errlen ) {
	zWalker w = {0};
	memset( conn, 0, sizeof( zdbconn_t ) );

	int psize = 0;
	for ( char ptr[ 256 ], *val; strwalk( &w, connstr, "=,; " ); ) {
		if ( w.chr == '=' ) {
			memset( ptr, 0, sizeof( ptr ) );
			memcpy( ptr, w.src, psize = w.size - 1 );
		}
		else if ( w.size > 1 ) { 
			int size = w.size;
			//val = w.src;
			if ( !strcmp( ptr, "user" ) || !strcmp( ptr, "username" ) )
				memcpy( (void *)conn->username, w.src, size - 1 );
			else if ( !strcmp( ptr, "pw" ) || !strcmp( ptr, "passw" ) || !strcmp( ptr, "password" ) )
				memcpy( (void *)conn->password, w.src, size - 1 );
			else if ( !strcmp( ptr, "hostname" ) || !strcmp( ptr, "host" ) )
				memcpy( (void *)conn->hostname, w.src, size - 1 );
			else if ( !strcmp( ptr, "dbname" ) || !strcmp( ptr, "database" ) || !strcmp( ptr, "db" ) )
				memcpy( (void *)conn->dbname, w.src, size - 1 );
			else if ( !strcmp( ptr, "unixsock" ) )
				memcpy( (void *)conn->unixsock, w.src, size - 1 );
			else {
		#if 1
				char buf[ 32 ] = { 0 };
				memcpy( buf, w.src, !strchr( ":;,", w.chr ) ? size : size - 1 );
				if ( !strcmp( "port", ptr ) ) {
					if ( zdb_check_number( buf, strlen( buf ) ) )  
						conn->port = atoi( buf );
					else {
						snprintf( err, errlen - 1, "Port either not a number or is invalid." ); 
						return NULL;
					}
				}
				else if ( !strcmp( "timeout", ptr ) ) {
					if ( zdb_check_number( buf, strlen( buf ) ) )  
						conn->timeout = atoi( buf );
					else {
						snprintf( err, errlen - 1, "Timeout either not a number or is invalid." ); 
						return NULL;
					}
				}
			}
		#endif
			memset( ptr, 0, sizeof( ptr ) );	
		}
	}

	return conn; 
}


#ifdef ZTABLE_H
zTable * zdb_to_ztable ( zdb_t *zdb, const char *key ) {
	zTable *t = NULL;
	//TODO: This is going to eat some memory...
	const int mod = zdb->mapsize * 4;
	int next = 0, row = 0;

	//TODO: mod needs to be based on the result count
	if ( !( t = malloc( sizeof( zTable ) ) ) || !memset( t, 0, sizeof( zTable ) ) ) {
		return NULL;
	}

	if ( !lt_init( t, NULL, mod ) ) {
		return NULL;
	}

	//Start the table up
	lt_addtextkey( t, key );
	lt_descend( t );

	//Loop through and save each value 
	for ( zdbv_t **set = zdb->results; set && *set; set++ ) {
		if ( !next ) {
			lt_addintkey( t, row++ );
			lt_descend( t );
			next = 1;
		}

		//TODO: either method works, but not sure how often I'm doing this yet...
		//if ( AT_EOS( *set ) ) {
		if ( (*set)->len == -1 && *(*set)->field == eos ) { 
			lt_ascend( t );
			next = 0;
			continue;	
		}

		lt_addtextkey( t, (*set)->field );
		( !(*set)->len ) ? lt_addtextvalue( t, "" ) : lt_addblobdvalue( t, (*set)->value, (*set)->len );
		lt_finalize( t );
	}
	
	//Close the table and lock it
	lt_ascend( t );
	lt_lock( t );
	return t; 
}
#endif



#ifdef ZDB_ENABLE_SQLITE 
//...
void *zdb_sqlite_exec( zdb_t *zdb, const char *query, zdbv_t **records ) {

	//Define a bunch of stuff...
	int clsize = 0, columnCount = 0, status = 0;
	sqlite3_stmt *stmt = NULL;
	const char *unused = NULL; 

	//Prepare the statment
	if ( sqlite3_prepare_v2( zdb->ptr, query, -1, &stmt, &unused ) != SQLITE_OK ) {
		const char * err = sqlite3_errmsg( zdb->ptr );
		snprintf( zdb->err, ZDB_ERRBUF_LEN, "sqlite3 prepare error: %s\n", err );
		zdb->error = ZDB_ERROR_PREPARE;
		return NULL;
	}

	//Binding might need to happen here...
	if ( records ) {
		zdbv_t *bindargs[ ZDB_MAX_BINDS ] = { NULL };
		zdbv_t **r = records;

		//Unfortunately, SQLite needs the bind args in order...
		for ( int bindex; r && *r; r++ ) {
			if (( bindex = sqlite3_bind_parameter_index( stmt, (*r)->field ) ) > ZDB_MAX_BINDS ) {
				zdb->error = ZDB_ERROR_BINDMAX;
				sqlite3_finalize( stmt );
				return NULL;
			}

			if ( bindex < 1 ) {
				zdb->error = ZDB_ERROR_BINDPARAM;
				snprintf( zdb->err, ZDB_ERRBUF_LEN, zdb_errors[ ZDB_ERROR_BINDPARAM ], (*r)->field ); 
				sqlite3_finalize( stmt );
				return NULL;
			}
			
			bindargs[ bindex ] = *r;
		}

		for ( int i = 1; bindargs[i] != NULL ; i++ ) {	
			zdbv_t *a = bindargs[ i ];
			status = sqlite3_bind_text( stmt, i, a->value, a->len, SQLITE_STATIC );
			if ( status != SQLITE_OK ) {
				zdb->error = ZDB_ERROR_BIND;
				snprintf( zdb->err, ZDB_ERRBUF_LEN, zdb_errors[ ZDB_ERROR_BIND ], a->field ); 
				sqlite3_finalize( stmt );
				return NULL;
			}
		}
	}

	//Get the column names (try to clone just once)
	if ( ( columnCount = sqlite3_column_count( stmt ) ) > ZDB_MAX_COLUMNS ) {
		snprintf( zdb->err, ZDB_ERRBUF_LEN, "RESULT SET COLUMN COUNT TOO LARGE ( >%d )\n", ZDB_MAX_COLUMNS );
		zdb->error = ZDB_ERROR_COLUMNLENGTHEXCEEDMAX;
		sqlite3_finalize( stmt );
		return NULL;
	}

	//Bind all arguments...
	for ( int i=0; i < columnCount; i++ ) {
		const char *col = zdb_dupstr(  (void *)sqlite3_column_name( stmt, i ) );
		zdbv_add_item( &zdb->headers, (void *)col, char *, &clsize );	
	}

	//Save the results...
	int size = 0;
	for ( int results; ( status = sqlite3_step( stmt ) ) != SQLITE_DONE; zdb->rows++ ) {
		if ( status != SQLITE_ROW ) {
			snprintf( zdb->err, ZDB_ERRBUF_LEN, zdb_errors[ ZDB_ERROR_QUERY ], sqlite3_errmsg( zdb->ptr ) );
			zdb->error = ZDB_ERROR_QUERY;
			sqlite3_finalize( stmt );
			return NULL;
		} 

		for ( int i=0, dc = sqlite3_data_count( stmt ), len; i <= dc; i++ ) {
			//Create a new thing
			zdbv_t *val = malloc( sizeof( zdbv_t ) );
			memset( val, 0, sizeof( zdbv_t ) );
	
			if ( i == dc )
				val->field = endset, val->len = -1, val->value = NULL;
			else {
				//Set values
				val->field = zdb->headers[ i ];
				val->len = sqlite3_column_bytes( stmt, i );	
				val->value = malloc( val->len );
				memset( val->value, 0, val->len );
				memcpy( val->value, sqlite3_column_blob( stmt, i ), val->len );
			}
			
			if ( !zdbv_add_item( &zdb->results, val, zdbv_t *, &size ) ) {
				zdb->error= ZDB_ERROR_ALLOC;
				sqlite3_finalize( stmt );
				return NULL;
			}
		}
	}

	//Mark the structure when done
	zdb->affected = sqlite3_changes( zdb->ptr );
	zdb->mapsize = columnCount * zdb->rows;

	sqlite3_finalize( stmt );
	return zdb->results;
}




void *zdb_sqlite_open( zdb_t *zdb, const char *string, char *err, int errlen ) {
	sqlite3 *ptr = NULL;
	if ( sqlite3_open( string, &ptr ) != SQLITE_OK ) {
		snprintf( err, errlen, "%s", sqlite3_errmsg( ptr ) ); 
		return NULL;	
	}
	return ptr;
}



void *zdb_sqlite_close( void **ptr, char *err, int errlen ) {
	if ( sqlite3_close( *ptr ) != SQLITE_OK ) {
		snprintf( err, errlen, "%s", sqlite3_errmsg( *ptr ) ); 
		return NULL;	
	}
	*ptr = NULL;
	return (int *)1;
}
#endif

#ifdef ZDB_ENABLE_MYSQL
void *zdb_mysql_open( zdb_t *zdb, const char *string, char *err, int errlen ) {
	MYSQL *conn = NULL;
	if ( !( conn = malloc( sizeof( MYSQL ) ) ) || !memset( conn, 0, sizeof( MYSQL ) ) ) {
		zdb->error = ZDB_ERROR_ALLOC;
		return NULL;
	}

	//Not sure why this would fail...
	if ( !zdb_init_conn( &zdb->conn, string, zdb->err, ZDB_ERRBUF_LEN ) ) {
		zdb->error = ZDB_ERROR_CONNSTRING;
		mysql_close( conn );
		free( conn );
		return NULL;
	}

	//This is probably an allocation failure
	if ( !mysql_init( conn ) ) {
		zdb->error = ZDB_ERROR_NONE;
		mysql_close( conn );
		free( conn );
		return NULL;
	}

	if ( !mysql_real_connect( conn, 
		zdb->conn.hostname,
		zdb->conn.username,
		zdb->conn.password,
		zdb->conn.dbname,
		zdb->conn.port,
		NULL, //!*zdb->conn.unixsock ? zdb->conn.unixsock : "",
		0 ) )
	{
		snprintf( zdb->err, ZDB_ERRBUF_LEN, "%s", mysql_error( conn ) );
		zdb->error = ZDB_ERROR_OPEN;
		mysql_close( conn );
		free( conn );
		return NULL;		
	}

	return ( zdb->ptr = conn );
}



void *zdb_mysql_close( void **ptr, char *err, int errlen ) {
	mysql_close( *ptr ); 
	free( *ptr );
	return (int *)1;
}



void * zdb_mysql_exec( zdb_t *zdb, const char *query, zdbv_t **records ) {
	MYSQL_RES *result;
	MYSQL_ROW row;
	MYSQL_STMT *stmt;
	int paramcount = 0;
	const char *nquery = query; //Bind statements need this...
	const char bindname = ':';
	const char **bindparams = NULL;
	const char bpset[] = ":,' ";

	//Initialize
	if ( !( stmt = mysql_stmt_init( zdb->ptr ) ) ) {
		zdb->error = ZDB_ERROR_ALLOC;
		return NULL;
	}

#if 0
#error "MySQL Bind is ineffective right now"

//To fix this, 
//1. Copy the query without the named parameters and use the question marks
//2. Keep a count of how many bind parameters we got (I feel like you'll need it)
//3. Allocate an array of values matching the number of params we got
//(This is already done)
//4. Loop through the array, and check in zdbv_t ** for the matching field
//	Replace with value (free the original, and (possibly) replace it w/ a 
//	copy of zdbv_t value... can't tell if this is unnecessary or not)
//5. Allocate the correct number of MYSQL_BIND structures needed with the
//	values from the above array...
//6. Make sure it throws if the values don't match...
//*. Building type safety in at zdbv_t is easier than going the other way...
#endif
	//Find ':' within the query
	if ( strchr( query, ':' ) ) {
		//Set up all the bind arguments by name
		unsigned char *qp = malloc( strlen( query ) + 1 );
		memset( (void *)qp, 0, strlen( query ) + 1 );
		nquery = (const char *)qp;
		zWalker w = { 0 };
		int tok = 0, len = 0, bplen = 0;

		for ( ; strwalk( &w, query, bpset ); ) {
		//fprintf( stderr, "chr: '%c' ", w.chr );write( 2, w.src, w.size );
			if ( w.chr == '\'' )
				tok = 1;
			else if ( w.chr == ':' ) {
				*qp = '?', qp++, tok = 1, len += 1;
				continue;
			}
			else if ( tok == 1 ) {
				//whatever set of characters came after the token, is the named param we're looking for
				unsigned char buf[ 128 ];
				memset( buf, 0, sizeof( buf ) );
				memcpy( buf, w.src, !strchr( bpset, w.chr ) ? w.size : w.size - 1 );
				zdbv_add_item( &bindparams, (void *)zdb_dupstr( (char *)buf ), char *, &bplen );
				tok = !tok;
				continue;
			}

			//Catch all otherwise...
			memcpy( qp, w.src, w.size );
			qp += w.size, len += w.size;	
		}
	}

	//Prepare
	if ( mysql_stmt_prepare( stmt, nquery, strlen( nquery ) ) ) {
		zdb->error = ZDB_ERROR_PREPARE;
		snprintf( zdb->err, ZDB_ERRBUF_LEN - 1, "%s", mysql_error( zdb->ptr ) );
		return NULL;
	}

	//Get a count of parameters... (if > 0, then we probably need to bind something
	if ( records && ( paramcount = mysql_stmt_param_count( stmt ) ) > 0 ) {
		//Initialize the bind params		
		MYSQL_BIND bind[ paramcount ];
		memset( &bind, 0, sizeof( bind ) );

		//find the right code	
		for ( int match = 0, i = 0; i < paramcount; i ++ ) {
			//find the matching string
			for ( zdbv_t **r = records; r && *r; r ++ ) {
				//Account for the bind character ( ':' )
				if ( !strcmp( (*r)->field + 1, bindparams[ i ] ) ) {
					bind[ i ].buffer_type = MYSQL_TYPE_STRING;
					bind[ i ].buffer = (*r)->value;
					bind[ i ].buffer_length = (*r)->len + 1;
					bind[ i ].length = &(*r)->len;
					bind[ i ].is_null = 0;
					match = 1;
				}
			}
	
			if ( !match ) {
				//the named parameter was not found...
				snprintf( zdb->err, ZDB_ERRBUF_LEN, "%s", mysql_stmt_error( stmt ) );
				zdb->error = ZDB_ERROR_BINDPARAM;
				return NULL;	
			}
		}

		//Bind rhe parameters 
		if ( mysql_stmt_bind_param( stmt, bind ) ) {
			zdb->error = ZDB_ERROR_BIND;
			snprintf( zdb->err, ZDB_ERRBUF_LEN, "%s", mysql_stmt_error( stmt ) );
			return NULL;
		}
	}

	//Execute the query
	if ( mysql_stmt_execute( stmt ) ) {
		snprintf( zdb->err, ZDB_ERRBUF_LEN - 1, "%s", mysql_stmt_error( stmt ) );
		zdb->error = ZDB_ERROR_QUERY;
		return NULL;
	}

#if 1
	//Fetch metadata
	if ( !( result = mysql_stmt_result_metadata( stmt ) ) ) {
		/*
		zdb->error = ZDB_ERROR_QUERY;
		snprintf( zdb->err, ZDB_ERRBUF_LEN - 1, "%s", mysql_stmt_error( stmt ) );
		return NULL;
		*/
		//This can happen if there were no rows returned by query 
		//Check errno instead
	}
	else {
		//Get column count and make another bind structure for values
		int ccount = mysql_stmt_field_count( stmt );
		MYSQL_BIND resbind[ ccount ];
		unsigned long reslongs[ ccount ];
		memset( &resbind, 0, sizeof( resbind ) );
		memset( &reslongs, 0, sizeof( reslongs ) );
		zdb->results = NULL;

		if ( ccount ) {
			//Get each of the field names
			int fsize = 0;
			int reslen = 0;
			MYSQL_FIELD *f = NULL;
			for ( int fsize = 0, i = 0; ( f = mysql_fetch_field( result ) ); i ++ ) {
				zdbv_add_item( &zdb->headers, (void *)zdb_dupstr( f->name ), char *, &fsize );
			} 

			//Set up each of these structures the right way...
			for ( int i = 0; i < ccount; i++ ) {
				resbind[ i ].buffer = 0;
				resbind[ i ].buffer_length = 0;
				resbind[ i ].buffer_type = MYSQL_TYPE_STRING;
				resbind[ i ].length = &reslongs[ i ]; //malloc( sizeof (unsigned long) );
			}

			//Make the assumption that this is one time...
			if ( mysql_stmt_bind_result( stmt, resbind ) ) {
				zdb->error = ZDB_ERROR_GENERIC;
				snprintf( zdb->err, ZDB_ERRBUF_LEN, "stmt_bind_result: %s", mysql_stmt_error(stmt) );
				return NULL;
			}

			//Get all of the results
			for ( int iter = 0; iter < 1; iter++ ) {
				//Do the first fetch
				int status = mysql_stmt_fetch( stmt );
				if ( status == MYSQL_NO_DATA ) {
					//Do other things...
					break;
				}
				else if ( status == 1 ) {
					zdb->error = ZDB_ERROR_GENERIC;
					snprintf( zdb->err, ZDB_ERRBUF_LEN, "stmt_fetch: %s", mysql_stmt_error(stmt) );
					return NULL;
				}

				//Prepare each buffer
				for ( int i = 0; i < ccount; i++ ) {
					//Allocate space for a new one and save the column name
					unsigned char *buf = NULL;
					zdbv_t *p = malloc( sizeof( zdbv_t ) );
					memset( p, 0, sizeof( zdbv_t ) );
					p->field = zdb->headers[ i ];
					p->value = NULL;
					p->len = resbind[ i ].buffer_length = *resbind[ i ].length; 

					//Even if there is no value, we need to save...
					if ( p->len ) { 
						resbind[ i ].buffer = p->value = malloc( resbind[ i ].buffer_length ); 
						memset( p->value, 0, p->len );
					}
					zdbv_add_item( &zdb->results, p, zdbv_t *, &reslen ); 
					fprintf( stderr, "%s -> ptr: %p, len: %ld\n ", p->field, p->value, p->len );
				}

				//Finally, fetch all of the data from each column.
				for ( int i = 0; i < ccount; i ++ ) {
					if ( mysql_stmt_fetch_column( stmt, resbind, i, 0 ) ) {
						zdb->error = ZDB_ERROR_GENERIC;
						snprintf( zdb->err, ZDB_ERRBUF_LEN, "stmt_fetch_column: %s", mysql_stmt_error(stmt) );
						return NULL;
					}
				}
			}
			zdbv_dump( zdb->results );
		}

		zdb->rows = mysql_stmt_num_rows( stmt );
		zdb->affected = mysql_stmt_affected_rows( stmt );
		mysql_free_result( result );
	}
#endif	
	
#if 0
	//Interpret this metadata?
	zdb->rows = mysql_stmt_num_rows( stmt );
	zdb->affected = mysql_stmt_affected_rows( stmt );
	fprintf( stderr, "num: %d, aff: %d\n\n", zdb->rows, zdb->affected );
#endif

#if 0
	//Handle any error
	if ( mysql_errno( zdb->ptr ) ) {
		snprintf( zdb->err, ZDB_ERRBUF_LEN, "Query error: %s", mysql_error( zdb->ptr ) );
	}
#endif

	if ( mysql_stmt_close( stmt ) ) {
		zdb->error = ZDB_ERROR_STMTCLOSE;
		return NULL;
	}

	return (int *)1;
}
#endif




#ifdef DEBUG_H
void zdbconn_print ( zdbconn_t *conn ) {
	fprintf( stderr, "username: %s\n", conn->username );
	fprintf( stderr, "password: %s\n", conn->password );
	fprintf( stderr, "hostname: %s\n", conn->hostname );
	fprintf( stderr, "dbname: %s\n", conn->dbname );
	fprintf( stderr, "port: %d\n", conn->port );
	fprintf( stderr, "unixsock: %s\n", conn->unixsock );
	fprintf( stderr, "options: %p\n", conn->options );
	//fprintf( stderr, "flags: %p\n", conn->flags );
}
#endif


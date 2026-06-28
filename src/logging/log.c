/**
 * log.c
 * -------
 *
 * Logging utilities for regular files and SQLite3.
 *
 */
#include "log.h"


// Create format
static const char CREATEFMT[] =
	"'%s','%s',%d,%d,%d,%d,'%s','%s','%s','%s',%d,%d,%d,'%c'"
;

// Insert format
static const char INSERTFMT[] =
		"INSERT INTO access_log VALUES "
		"( NULL,'%s','%s','%s',%f,'%s','%s','%s','%s',%d,%ld,%d,'%c','%s','%s','%s','%s','%s','%s','%s' )"
;

// Create statement for a log
static const char CHECK[] =
	"SELECT * FROM access_log LIMIT 1;"
;


// Create statement for a log
static const char CREATE[] = " \
	CREATE TABLE access_log ( \
		iindex INTEGER PRIMARY KEY AUTOINCREMENT, \
		ipv4 TEXT, \
		ipv6 TEXT, \
		date TEXT, \
		proctime REAL, \
		host TEXT, \
		uri TEXT, \
		prot TEXT, \
		method TEXT, \
		status INTEGER, \
		clength INTEGER, \
		port INTEGER, \
		type TEXT, \
		useragent TEXT,\
		extra_1 TEXT, \
		extra_2 TEXT, \
		extra_3 TEXT, \
		extra_4 TEXT, \
		extra_5 TEXT, \
		extra_6 TEXT \
	); \
";

// Get the time elapsed for a request.
static double diff_time ( const struct timespec *start, const struct timespec *end ) {
	if ( end->tv_sec == start->tv_sec ) {
		return (double)(( end->tv_nsec - start->tv_nsec ) / (double)1e9);
	}

	//long sec = ( end->tv_sec - 1 ) - start->tv_sec;
	//long nsec = ( 1e9 - start->tv_nsec ) + end->tv_nsec;
	return (double)(( end->tv_sec - 1 ) - start->tv_sec ) + ( (double)(( 1e9 - start->tv_nsec ) + end->tv_nsec) / 1e9 );
}



/**
 * int f_open( char *name, void **d )
 *
 * Open the object.
 *
 */
int f_open( const char *name, void **d, char *err, int errlen ) {
	FILE *f = NULL;
	if ( !( f = fopen( name, "a" ) ) ) {
		snprintf( err, errlen, "Could not open log file '%s': %s", name, strerror( errno ) );
		return 0;
	}
	*d = f;
	return 0;
}




/**
 * int f_close( void *d )
 *
 * Close the object.
 *
 */
int f_close( void *d ) {
	FILE *f = (FILE *)d;
	return fclose( f ) != -1;
}



/**
 * int f_write( void *d )
 *
 * Write to it.
 *
 */
int f_write( void *d, const server_t *server, const conn_t *conn ) {
	FILE *f = (FILE *)d;
	long nsec = ( 1e9 - conn->start.tv_nsec ) + conn->end.tv_nsec;
	// Do an snprintf? (I feel like this is crazy slow)
	fprintf( f,
		"'%s';'%s';%ld;%ld;%ld;%ld;'%s';'%s';'%s';'%s';%d;%ld;%d;'%c'",
		conn->ipv4,
		"", /*NULL,*/
		conn->start.tv_sec,
		conn->start.tv_nsec,
		conn->end.tv_sec,
		conn->end.tv_nsec,
		conn->config->name,
		conn->req->path,
		conn->req->protocol,
		conn->req->method,
		conn->req->status,
		conn->req->clen,
		conn->req->port,
		'E'
	);
	return 1;
}



/**
 * int sqlite3_log_open ( const char *, void **, void *, int )
 *
 * Open the SQLite3 logging database.
 *
 */
int sqlite3_log_open ( const char *name, void **d, char *err, int errlen ) {
	sqlite3 *ppdb = NULL;
	char  *lerr = NULL;

	//open a sqlite3 handle
	if ( sqlite3_open( name, &ppdb ) != SQLITE_OK ) {
		snprintf( err, errlen, "Connection failed: %s\n", sqlite3_errmsg( ppdb ) );
		return 0;
	}

	//check for the table, if it does not exist, create it
	if ( sqlite3_exec( ppdb, CHECK, NULL, NULL, &lerr ) != SQLITE_OK ) {
		if ( sqlite3_exec( ppdb, CREATE, NULL, NULL, &lerr ) != SQLITE_OK ) {
			snprintf( err, errlen, "Log table creation failed: %s\n", sqlite3_errmsg( ppdb ) );
			return 0;
		}
	}

	//set the sqlite3 handle
	*d = (void *)ppdb;
	return 1;
}



/**
 * int sqlite3_log_write( void *, const server_t *, const conn_t * )
 *
 * Write to an open SQLite handle.
 *
 */
int sqlite3_log_write( void *dd, const server_t *server, const conn_t *conn ) {

	sqlite3 *db = NULL;
	const char timefmt[] = "%F %T %z";
	char *err = NULL;
	char code = 'A';
	char timebuf[ 128 ] = {0};
	char pathbuf[ 2048 ] = {0};
	char sqlbuf[ 4096 ] = {0};
	long nsec = ( 1e9 - conn->start.tv_nsec ) + conn->end.tv_nsec;
	struct tm tmp;
	char ua[ 1024 ] = {0};
	const time_t t = time( NULL );
	localtime_r( &t, &tmp );

	// Check for a live reference
	if ( !( db = server->logger->data ) ) {
		FPRINTF( "db handle disappeared.  Log this as a fatal error...\n" );
		return 0;
	}

	// Time
	if ( !strftime( timebuf, sizeof( timebuf ) - 1, timefmt, &tmp ) ) {
		FPRINTF( "strftime failed.  Log this...\n" );
	}

	// If this is not done for whatever reason, it's an error...
	if ( !conn->config ) {
		FPRINTF( "No host found, fatal...\n" );
		// TODO: TLS failures seem to get us here the fastest, logging it is still useful
		snprintf( sqlbuf, sizeof( sqlbuf ) - 1, INSERTFMT,
			conn->ipv4,
			"-", /* ipv6, not supported yet */
			timebuf,  /*NULL,*/
			diff_time( &conn->start, &conn->end ),
			"-",
			conn->req->path,
			"HTTP/1.1", //conn->req->protocol,
			"-", //conn->req->method,
			-1, //conn->res->status,
			(long)-1, //conn->res->clen,
			-1, //conn->req->port,
			'E',
			"-",
			conn->err, // Get more info about the specific error here
			"",
			"",
			"",
			"",
			""
		);
		return 0;
	}

	// Check the status to check for failure
	if ( conn->res->status > 399 ) {
		code = 'E';
	}

	// You'll have to find the user-agent (if there is one. use '-' otherwise)
	for ( zhttpr_t **r = conn->req->headers; r && *r; r++ ) {
		if ( strcasecmp( "user-agent", (*r)->field ) == 0 && (*r)->size < sizeof( ua ) - 1 ) {
			memcpy( ua, (*r)->value, (*r)->size );
		}
	}

	// Set "unknown" user-agent
	if ( *ua == 0 ) {
		*ua = '-';	
	}

	// Do an snprintf? (I feel like this is crazy slow)
	snprintf( sqlbuf, sizeof( sqlbuf ) - 1, INSERTFMT,
		conn->ipv4,
		"-", /* ipv6, not supported yet */
		timebuf,  /*NULL,*/
		diff_time( &conn->start, &conn->end ),
		conn->config->name,
		conn->req->path,
		conn->req->protocol,
		conn->req->method,
		conn->res->status,
		conn->res->clen,
		conn->req->port,
		code,
		ua,
		conn->err,
		"",
		"",
		"",
		"",
		""
	);

	// Execute a query
	if ( sqlite3_exec( db, sqlbuf, NULL, NULL, &err ) != SQLITE_OK ) {
		FPRINTF( "Writing to logging table failed: %s\n", sqlite3_errmsg( db ) );
		FPRINTF( "BUFFER: %s\n", sqlbuf );
		return 0;
	}

	return 1;
}



/**
 * int sqlite3_log_close( void *d )
 *
 * Close the logging database.
 *
 */
int sqlite3_log_close( void *d ) {
#if 0
	return ( sqlite3_close( (sqlite3 *)d ) == SQLITE_OK );
#endif
	return 1;
}



#if 0
#ifdef DEBUG_H
void print_log_stmt ( const conn_t *conn ) {

	FPRINTF( INSERTFMT,
		conn->ipv4,
		"-", /* ipv6, not supported yet */
		timebuf,  /*NULL,*/
		diff_time( &conn->start, &conn->end ),
		"-",
		conn->req->path,
		conn->req->protocol, // "HTTP/1.1", //conn->req->protocol,
		conn->req->method, // "-", //conn->req->method,
		conn->res->status, // -1, //conn->res->status,
		conn->res->clen, // (long)-1, //conn->res->clen,
		conn->req->port, // -1, //conn->req->port,
		'E',
		"-",
		conn->err, // Get more info about the specific error here
		"",
		"",
		"",
		"",
		""
	);

}
#endif
#endif


#ifdef TEST_H
/**
 * int main (int argc, char *argv[]) 
 *
 * Tests that logging is working with a variety of values
 *
 */
int main (int argc, char *argv[]) {
	// TODO: Shell tools are going to be the best idea for writing
	// tests 9/10 times.  Write a very simple program that can open
	// a file, read input and act upon it
	return 0;
}
#endif

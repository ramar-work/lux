/* -------------------------------------------------------- *
 * server.c
 * ========
 * 
 * Summary 
 * -------
 * Server functions for Hypno's server
 * 
 * Usage
 * -----
 * These are the options:
 *   --start            Start new servers             
 *   --debug            Set debug rules               
 *   --kill             Test killing a server         
 *   --fork             Daemonize the server          
 *   --config <arg>     Use this file for configuration
 *   --port <arg>       Set a differnt port           
 *   --ssl              Use ssl or not..              
 *   --user <arg>       Choose a user to start as     
 * 
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 * 
 * Changelog 
 * ----------
 * - 
 * -------------------------------------------------------- */
#include "server.h"

//Check the validity of a filter
static struct filter * srv_check_filter ( const struct filter *filters, char *name ) {
	while ( filters && filters->name ) {
		struct filter *f = ( struct filter * )filters;
		if ( f->name && strcmp( f->name, name ) == 0 ) {
			return f;
		}
		filters++;
	}
	return NULL;
}



//Check that the website's chosen directory is accessible and it's log directory is writeable.
static int srv_check_dir ( struct cdata *conn, char *err, int errlen ) {
	FPRINTF( "Checking directory at %s\n", conn->hconfig->dir );

	//Check that log dir is accessible and writeable (or exists) - send 500 if not 
	struct stat sb;
	char *adir, dir[ 2048 ] = { 0 };

	if ( !( adir = conn->hconfig->dir ) ) {
		snprintf( err, errlen, "Directory for host '%s' does not exist.", conn->hconfig->name );
		return 0;
	}

	if ( stat( adir, &sb ) == -1 ) {
		//Try to build a full one
		snprintf( dir, sizeof(dir), "%s/%s", conn->config->wwwroot, conn->hconfig->dir );
		if ( stat( dir, &sb ) == -1 ) {
			const char *fmt = "Directory for host '%s' not accessible: %s.";
			snprintf( err, errlen, fmt, conn->hconfig->name, strerror(errno) );
			return 0;
		}
		adir = dir;
	}

	if ( access( adir, W_OK ) == -1 ) {
		const char *fmt = "Directory for host '%s' not writeable: %s.";
		snprintf( err, errlen, fmt, conn->hconfig->name, strerror(errno) );
		return 0;
	}

	//TODO: Fix me
	free( conn->hconfig->dir );
	conn->hconfig->dir = NULL;
	conn->hconfig->dir = strdup( adir );
	return 1;
}


//Build server configuration
static const int srv_start( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Initial server allocation started...\n" );
	char err[ 2048 ] = {0};
	
	conn->config = build_server_config( conn->ctx->config, err, sizeof(err) );
	if ( !conn->config ) { 
		conn->count = -3;
		return http_set_error( rs, 500, err );
	}

	FPRINTF( "Initial server allocation complete.\n" );
	return 1;
}


//Find the chosen host and generate a response via one of the selected filters
static const int srv_proc( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn) {
	FPRINTF( "Proc started...\n" );
	char err[2048] = {0};
	zTable *t = NULL;
	struct filter *filter = NULL;
	int count = conn->count;	
	conn->count = -3;

	//With no default host, throw this 
	if ( !req->host ) {
		snprintf( err, sizeof(err), "No host header specified." );
		return http_set_error( res, 500, err ); 
	}

	if ( !( conn->hconfig = find_host( conn->config->hosts, req->host ) ) ) {
		snprintf( err, sizeof(err), "Could not find host '%s'.", req->host );
		return http_set_error( res, 404, err ); 
	}

	if ( !conn->hconfig->filter ) {
		snprintf( err, sizeof( err ), "No Filter specified for '%s'", conn->hconfig->name );
		return http_set_error( res, 500, err ); 
	}

	if ( !( filter = srv_check_filter( conn->ctx->filters, conn->hconfig->filter ) ) ) {
		snprintf( err, sizeof( err ), "Filter '%s' not supported", conn->hconfig->filter );
		return http_set_error( res, 500, err ); 
	}

	if ( conn->hconfig->dir && !srv_check_dir( conn, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err ); 
	}

	//Finally, now we can evalute the filter and the route.
	if ( !filter->filter( fd, req, res, conn ) ) {
		return 0;
	}

	//You can add a header to tell things to close
	conn->count = count;
	FPRINTF( "Proc complete.\n" );
	return 1;
}


//Generate a message in common log format: ip - - [date] "method path protocol" status clen
static const int srv_log( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn) {
	const char fmt[] = "%s %s %s [%s] \"%s %s %s\" %d %d";
	const char datefmt[] = "%d/%b/%Y:%H:%M:%S %z";
	char log[2048] = {0};
	char date[2048] = {0};

	//Generate the time	
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	strftime( date, sizeof(date), datefmt, tmp );

	//Bugs?
	char *prot = ( rq->protocol ) ? rq->protocol : "HTTP/1.1";	

	//Just print the log for now
	snprintf( log, sizeof(log), fmt, 
		conn->ipv4, "-", "-", date, rq->method, rq->path, prot, rs->status, rs->clen );
	FPRINTF( "%s\n", log );
	return 1;
}


//End the request by deallocating all of the things
static int srv_end( struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Deallocation started...\n" );

	//Free HTTP request 
	http_free_body( rq );

	//Free HTTP response
	http_free_body( rs );

	//Free the server config too
	free_server_config( conn->config );

	FPRINTF( "Deallocation complete.\n" );
	return 0;
}


//Generate a response
int srv_response ( int fd, struct cdata *conn ) {

	struct HTTPBody rq = {0}, rs = {0};
	char err[2048] = {0};
	int status = 0;

	const int (*rc[])( int, struct HTTPBody *, struct HTTPBody *, struct cdata *) = {
		srv_start
	, conn->ctx->pre
	, conn->ctx->read
	, srv_proc
	, conn->ctx->write
	, conn->ctx->post
	, srv_log
	};

	//A conn->count of -3 means stop and write the message immediately
	for ( int i = 0; i < 7; i++ ) {
		//if ( !status && conn->count == -3 && ( i = 3 ) )
		if ( !( status = rc[i]( fd, &rq, &rs, conn ) ) && conn->count == -3 && ( i = 3 ) )
			continue;
		else if ( !status && conn->count == -2 ) {
			srv_end( &rq, &rs, conn );
			return 0;
		}
	}

	//End the request and return a status
	srv_end( &rq, &rs, conn );

	//Request should finish before we count this as an attempt. 
	if ( conn->count >= 0 ) {
		conn->count ++;
	}
	return 1; 
}



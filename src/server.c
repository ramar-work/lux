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
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
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
 * 
 * Changelog 
 * ----------
 * 
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
static int srv_check_dir ( struct host *host, char *err, int errlen ) {
	FPRINTF( "Checking directory at %s\n", host->dir );

	//Check that log dir is accessible and writeable (or exists) - send 500 if not 
	struct stat sb;
	if ( !host->dir ) {
		snprintf( err, errlen, "Directory for host '%s' does not exist.", host->name );
		return 0;
	}

	//This check belongs within the filter...	
	if ( stat( host->dir, &sb ) == -1 ) {
		const char *fmt = "Log directory for host '%s' not accessible: %s.";
		snprintf( err, errlen, fmt, host->dir, strerror(errno) );
		return 0;
	}

	if ( access( host->dir, W_OK ) == -1 ) {
		const char *fmt = "Log directory for host '%s' not writeable: %s.";
		snprintf( err, errlen, fmt, host->name, strerror(errno) );
		return 0;
	}
	return 1;
}


//Start the request by allocating things we'll always need.
static int srv_start( struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Initial server allocation started...\n" );

	//Build server config (because it can change)
	char err[ 2048 ] = {0};
	
	conn->config = build_server_config( conn->ctx->config, err, sizeof(err) );
	if ( !conn->config ) { 
		//return http_set_error( res, 500, err );
		//Kill the context
		//Close the file
		//Show me what happened and kill it...
		return 0;
	}

	FPRINTF( "Initial server allocation complete.\n" );
	return 1;
}


//Find the chosen host and generate a response via one of the selected filters
static int srv_proc( struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn) {
	FPRINTF( "Proc started...\n" );
	char err[2048] = {0};
	zTable *t = NULL;
	struct filter *filter = NULL;

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

	if ( conn->hconfig->dir && !srv_check_dir( conn->hconfig, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err ); 
	}

	//Finally, now we can evalute the filter and the route.
	if ( !filter->filter( req, res, conn ) ) {
		return 0;
	}

	//You can add a header to tell things to close
	FPRINTF( "Proc complete.\n" );
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



//Generate a message in common log format: ip - - [date] "method path protocol" status clen
static void srv_log( struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn) {
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
	snprintf( prot, strlen(rq->protocol) + 1, "%s", rq->protocol );

	//Just print the log for now
	snprintf( log, sizeof(log), fmt, 
		"127.0.0.1", "-", "-", date, rq->method, rq->path, prot, rs->status, rs->clen );
	FPRINTF( "%s\n", log );
}


//Generate a response
int srv_response ( int fd, struct cdata *conn ) {

	struct HTTPBody rq = {0}, rs = {0};
	//struct config *config = NULL;
	char err[2048] = {0};
	int status = 0;

	//Initialize the userdata
	//conn->ctx = ctx;
	conn->status = 0;

	//Parse our configuration here...
	status = srv_start( &rq, &rs, conn );
	if ( !status && conn->count == -2 ) {
		srv_end( &rq, &rs, conn );
		return 0;
	}

#if 0
	//This is necessary for SSL
	//Do any per-request initialization here.
	if ( conn->ctx->pre ) {
		if ( !( status = conn->ctx->pre( fd, config, &conn->ctx->data ) ) ) {

		}
	}
#endif

	//Read the message
	status = conn->ctx->read( fd, &rq, &rs, (void *)conn );
	if ( !status && conn->count == -2 ) {
		srv_end( &rq, &rs, conn );
		return 0;
	}

	//Generate a message
	status && ( status = srv_proc( &rq, &rs, conn ) );
	#if 0
	if ( !status && conn->count == -2 ) {
		return 0;
	}
	#endif

	//Write the message	(should almost ALWAYS run)
	//status = ctx->write( fd, &rq, &rs, ctx->data );
	//status = ctx->write( fd, &rq, &rs, (void *)conn );
	if ( !status && conn->count == -2 ) {
		srv_end( &rq, &rs, conn );
		return 0;
	}

	//Log the server problems (unless they were just bugs)
	srv_log( &rq, &rs, conn );

	//Per-request shut down goes here.
	//ctx->post && ctx->post( fd, config, &ctx->data );

#if 1
	//End the request and return a status
	srv_end( &rq, &rs, conn );

	//Request should finish before we count this as an attempt. 
	conn->count ++;
#else
#endif
	return 1; 
}



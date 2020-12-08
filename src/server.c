/* -------------------------------------------------------- *
server.c
========

This is the basis of hypno's web server.

USAGE
-----
These are the options:
  --start            Start new servers             
  --debug            Set debug rules               
  --kill             Test killing a server         
  --fork             Daemonize the server          
  --config <arg>     Use this file for configuration
  --port <arg>       Set a differnt port           
  --ssl              Use ssl or not..              
  --user <arg>       Choose a user to start as     


BUILDING
--------
Lua is a necessity because of the configuration parsing. 

Running make is all we need for now on Linux and OSX.  Windows
will need some additional help and Cygwin as well.


TODO
----
- Implement threaded model
- Write a couple of different types of loggers
- Add global root default for config.lua
- Is it useful to move the configuration initialization to a
  different part of the code.
- Add an option to show a parsed config

 * -------------------------------------------------------- */
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "../vendor/zhttp.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "luabind.h"
#include "socket.h"
#include "util.h"
#include "ctx-http.h"
#include "ctx-https.h"
#include "server.h"
#include "config.h"
#include "filter-static.h"
#if 0
#include "filter-dirent.h"
#include "filter-echo.h"
#include "filter-lua.h"
#include "filter-c.h"
#endif


//Check the validity of a filter
static struct filter * check_filter ( struct filter *filters, char *name ) {
	while ( filters && filters->name ) {
		struct filter *f = filters;
		if ( f->name && strcmp( f->name, name ) == 0 ) {
			return f;
		}
		filters++;
	}
	return NULL;
}


//Check that the website's chosen directory is accessible and it's log directory is writeable.
static int check_dir ( struct host *host, char *err, int errlen ) {
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
static int srv_start( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct config **config ) {
	FPRINTF( "Initial server allocation started...\n" );
	//Build a config
	char err[2048] = {0};
 #if 0
	if (( *config = build_config( err, sizeof(err) )) == NULL ) {
		//return http_set_error( res, 500, err );
		//Kill the context
		//Close the file
		//Show me what happened and kill it...
		return 0;
	}
 #endif 
	FPRINTF( "Initial server allocation complete.\n" );
	return 1;
}


//Find the chosen host and generate a response via one of the selected filters
#if 0
static int srv_proc( struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn) {
#else
static int srv_proc( struct HTTPBody *req, struct HTTPBody *res, struct config *config, struct senderrecvr *ctx ) {
#endif
	FPRINTF( "Proc started...\n" );
	char err[2048] = {0};
	zTable *t = NULL;
	struct host *host = NULL;
	struct filter *filter = NULL;

	//With no default host, throw this 
	if ( !req->host ) {
		snprintf( err, sizeof(err), "No host header specified." );
		return http_set_error( res, 500, err ); 
	}

	if ( !( host = find_host( config->hosts, req->host ) ) ) {
		snprintf( err, sizeof(err), "Could not find host '%s'.", req->host );
		return http_set_error( res, 404, err ); 
	}

	if ( !( filter = check_filter( ctx->filters, !host->filter ? "static" : host->filter ) ) ) {
		snprintf( err, sizeof( err ), "Filter '%s' not supported", host->filter );
		return http_set_error( res, 500, err ); 
	}

	if ( host->dir && !check_dir( host, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err ); 
	}

	//Finally, now we can evalute the filter and the route.
	if ( !filter->filter( req, res, config, host ) ) {
		return 0;
	}

	//You can add a header to tell things to close

	FPRINTF( "Proc complete.\n" );
	return 1;
}


//End the request by deallocating all of the things
static int srv_end( struct HTTPBody *rq, struct HTTPBody *rs, struct config *config ) {
	FPRINTF( "Deallocation started...\n" );

	//Free the HTTP body 
	http_free_body( rs );
	http_free_body( rq );

	//Free hosts
	free_hosts( config->hosts );

	//Free routes 
	free_routeh( config->routes );

	//Config should be freed here
	free_config( config );

	FPRINTF( "Deallocation complete.\n" );
	return 0;
}


//Sets all socket options in one place
int srv_setsocketoptions ( int fd ) {
	if ( fcntl( fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		FPRINTF( "fcntl error at child socket: %s\n", strerror(errno) ); 
		return 0;
	}
	return 1;
}


//Generate a message in common log format: 
//ip - - [date] "method path protocol" status clen
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
int srv_response ( int fd, struct senderrecvr *ctx, struct cdata *conn ) {

	struct HTTPBody rq = {0}, rs = {0};
	struct config *config = NULL;
	char err[2048] = {0};
	int status = 0;

	//Initialize the userdata
	conn->global = ctx->data;
	conn->scope = config;
	conn->status = 0;

	//Parse our configuration here...
#if 0
	status = srv_start( &rq, &rs, &config );
#else
	conn->scope = config = build_config( ctx->config, err, sizeof(err) ); 
	if ( !conn->scope ) {
		FPRINTF( "Could not parse configuration file at: %s\n", ctx->config );
		return 0;
	}
#endif

	//Do any per-request initialization here.
	//ctx->pre && ( status = ctx->pre( fd, config, &ctx->data ) );

	//Read the message
	//status = ctx->read( fd, &rq, &rs, ctx->data );
	status = ctx->read( fd, &rq, &rs, (void *)conn );
	if ( !status && conn->count == -2 ) {
		srv_end( &rq, &rs, config );
		return 0;
	}

	//Generate a message
	status && ( status = srv_proc( &rq, &rs, config, ctx ) );
	#if 0
	if ( !status && conn->count == -2 ) {
		return 0;
	}
	#endif

	//Write the message	(should almost ALWAYS run)
	//status = ctx->write( fd, &rq, &rs, ctx->data );
	status = ctx->write( fd, &rq, &rs, (void *)conn );
	if ( !status && conn->count == -2 ) {
		srv_end( &rq, &rs, config );
		return 0;
	}

	//Log the server problems (unless they were just bugs)
	srv_log( &rq, &rs, conn );

	//Per-request shut down goes here.
	ctx->post && ctx->post( fd, config, &ctx->data );

#if 1
	//End the request and return a status
	srv_end( &rq, &rs, config );

	//Request should finish before we count this as an attempt. 
	conn->count ++;
#else
#endif
	return 1; 
}



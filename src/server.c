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
struct filter * check_filter ( struct filter *filters, char *name ) {
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
int check_dir ( struct host *host, char *err, int errlen ) {
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


//Find the chosen host and generate a response via one of the selected filters
int srv_proc( struct HTTPBody *req, struct HTTPBody *res, struct config *config, struct senderrecvr *ctx ) {
	FPRINTF( "Proc started...\n" );
	char err[2048] = {0};
	zTable *t = NULL;
	struct host *host = NULL;
	struct filter *filter = NULL;

	//With no default host, throw this 
	if ( !req->host ) {
		char *e = "No host header specified.";
		FPRINTF( e );
		return http_set_error( res, 500, err ); 
	}
#if 0
	if (( config = build_config( err, sizeof(err) )) == NULL ) {
		return http_set_error( res, 500, err );
	}  
#endif
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

	FPRINTF( "Root default of host '%s' is: %s\n", host->name, host->root_default );
	//Finally, now we can evalute the filter and the route.
	if ( !filter->filter( req, res, config, host ) ) {
		return 0;
	}

	//print_httpbody( res );	
	//The thing needs to be freed 
	//free_config( config );
	FPRINTF( "Proc complete.\n" );
	return 1;
}


//Sets all socket options in one place
int srv_setsocketoptions ( int fd ) {
	if ( fcntl( fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		FPRINTF( "fcntl error at child socket: %s\n", strerror(errno) ); 
		return 0;
	}
	return 1;
}


//Serves as a logging function for now.
int srv_writelog ( int fd, struct sockAbstr *su ) {
#if 0
	struct sockaddr_in *cin = (struct sockaddr_in *)addrinfo;
	char *ip = inet_ntoa( cin->sin_addr );
	fprintf( stderr, "Got request from: %s on new file: %d\n", ip, fd );	
#endif
	return 1;
}

#if 0
//Start the request by allocating things we'll always need.
int srv_start( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct config **config ) {
	FPRINTF( "Initial server allocation started...\n" );
	//Build a config
	char err[2048] = {0};
	if (( *config = build_config( err, sizeof(err) )) == NULL ) {
		//return http_set_error( res, 500, err );
		//Kill the context
		//Close the file
		//Show me what happened and kill it...
		return 0;
	}
  
	FPRINTF( "Initial server allocation complete.\n" );
	return 1;
}


//End the request by deallocating all of the things
int srv_end( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct config *config ) {
	FPRINTF( "Deallocation started...\n" );

	//Close the file first
	if ( close( fd ) == -1 ) {
		FPRINTF( "Couldn't close child socket. %s\n", strerror(errno) );
	}

	//Free the HTTP body 
	http_free_body( rs );
	http_free_body( rq );

	//Destroy config...
	//free_config( config );

	FPRINTF( "Deallocation complete.\n" );
	return 0;
}
#endif



//Generate a message in common log format
void srv_commonlogformat( struct HTTPBody *rq, struct HTTPBody *rs ) {
// ip - - [date] "method path protocol" status clen
	const char fmt[] = 
		"%s %s %s [%s] \"%s %s %s\" %d %d";
	const char datefmt[] =
		"%d/%b/%Y:%H:%M:%S %z";
	char log[2048] = {0};
	char date[2048] = {0};

	//Generate the time	
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	strftime( date, sizeof(date), datefmt, tmp );

	//Bugs?
	char prot[127] = {0};
	snprintf( prot, strlen(rq->protocol), "%s", rq->protocol );

	//Just print the log for now
	snprintf( log, sizeof(log), fmt, 
		"127.0.0.1", "-", "-", date, rq->method, rq->path, prot, rs->status, rs->clen );
	FPRINTF( "%s\n", log );
}



//Generate a response
int srv_response ( int fd, struct senderrecvr *ctx ) {

	struct HTTPBody rq = {0}, rs = {0};
	struct config *config = NULL;
	char err[2048] = {0};
	int status = 0;

	//Parse our configuration here...
	//status = srv_start( fd, &rq, &rs, &config );
	if ( !( config = build_config( ctx->config, err, sizeof(err) ) ) ) {
		FPRINTF( "Could not parse configuration file at: %s\n", ctx->config );
		return 0;
	}	

	//Do any per-request initialization here.
	//ctx->pre && ( status = ctx->pre( fd, config, &ctx->data ) );

	//Read the message
	status = ctx->read( fd, &rq, &rs, ctx->data );

	//Generate a message
	status && ( status = srv_proc( &rq, &rs, config, ctx ) );

	//Write the message	(should almost ALWAYS run)
	ctx->write( fd, &rq, &rs, ctx->data );
	srv_commonlogformat( &rq, &rs );

	//Per-request shut down goes here.
	ctx->post && ctx->post( fd, config, &ctx->data );

	//End the request and return a status
	//srv_end( fd, &rq, &rs, config );
	//Forking may force me to free this in two places
	//ctx->free( &ctx->data );

	//Close new descriptor
	if ( close( fd ) == -1 ) {
		FPRINTF( "Couldn't close child socket. %s\n", strerror(errno) );
	} 

	//Free everything
	http_free_body( &rs );
	http_free_body( &rq );
	return 1; 
}



/* -------------------------------------------------------- *
server.c
========

This is the basis of hypno's web server.

USAGE
- 

BUILDING
- 

TODO
- Implement threaded model
- Write a couple of different types of loggers
- Add global root default for config.lua
- Is it useful to move the configuration initialization to a
  different part of the code.
 * -------------------------------------------------------- */
#include "../vendor/single.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "luabind.h"
#include "http.h"
#include "socket.h"
#include "util.h"
#include "filter-static.h"
#include "filter-dirent.h"
#include "filter-echo.h"
#include "filter-lua.h"
#include "filter-c.h"
#include "ctx-http.h"
#include "ctx-https.h"
#include "ctx-test.h"
#include "server.h"

#define CTXTYPE 1

int pctx ( int, struct HTTPBody *, struct HTTPBody *, void *);

struct senderrecvr sr[] = {
	{ pctx, read_notls, write_notls, accept_notls, free_notls, create_notls }
, { pctx, read_gnutls, write_gnutls, accept_gnutls, free_notls, create_gnutls }
, { NULL, read_static, write_static, accept_notls, free_notls  }
,	{ NULL }
};

struct filter filters[] = {
	{ "static", filter_static }
, { "dirent", filter_dirent }
, { "echo", filter_echo }
, { "lua", filter_lua }
, { "c", filter_c }
, { NULL }
};


extern const char http_200_custom[];
extern const char http_200_fixed[];
extern const char http_400_custom[];
extern const char http_400_fixed[];
extern const char http_500_custom[];
extern const char http_500_fixed[];

const int defport = 2000;
int arg_verbose = 0;
int arg_debug = 0;

//Find a host
struct host * find_host ( struct host **hosts, char *hostname ) {
	char host[ 2048 ] = { 0 };
	int pos = memchrat( hostname, ':', strlen( hostname ) );
	memcpy( host, hostname, ( pos > -1 ) ? pos : strlen(hostname) );
	while ( hosts && *hosts ) {
		struct host *req = *hosts;
		if ( req->name && strcmp( req->name, host ) == 0 )  {
			return req;	
		}
		if ( req->alias && strcmp( req->alias, host ) == 0 ) {
			return req;	
		}
		hosts++;
	}
	return NULL;
} 


//Check the validity of a filter
struct filter * check_filter ( struct filter *filters, char *name ) {
	while ( filters ) {
		struct filter *f = filters;
		if ( f->name && strcmp( f->name, name ) == 0 ) {
			return f;
		}
		filters++;
	}
	return NULL;
}


//Build global configuration
struct config * build_config ( char *err, int errlen ) {

	const char *funct = "build_config";
	struct config *config = NULL; 
	lua_State *L = luaL_newstate();
	Table *t = NULL;

	if ( ( config = malloc( sizeof ( struct config ) ) ) == NULL ) {
		snprintf( err, errlen, "Could not initialize memory at %s", funct );
		return NULL;
	}

	//After this conversion takes place, destroy the environment
	if ( !lua_exec_file( L, "www/config.lua", err, sizeof(err) ) ) {
		goto freeres;
		return NULL;
	}

	//Allocate a table for the configuration
	if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Could not initialize Table at %s\n", funct );
		goto freeres;
		return NULL;
	}

	//Convert configuration into a table
	if ( !lua_to_table( L, 1, t ) ) {
		snprintf( err, errlen, "Failed to convert Lua config data to table.\n" );
		goto freeres;
		return NULL;
	}

	//Build hosts
	if ( ( config->hosts = build_hosts( t ) ) == NULL ) {
		//Build hosts fails with null, I think...
		snprintf( err, errlen, "Failed to bulid hosts table.\n" );
		goto freeres;
		return NULL;
	}

#if 0
	//This is the global root default
	if ( ( config->root_default = get_char_value( t, "root_default" ) ) ) {
		config->root_default = strdup( config->root_default );
	} 
#endif

freeres:
	//Destroy lua_State and the tables...
	lt_free( t );
	free( t );
	lua_close( L );
	return config;
}


void free_config( struct config *config ) {
	struct host **hosts = config->hosts;
	while ( hosts && *hosts ) {
		struct host *h = *hosts;
		( h->name ) ? free( h->name ) : 0;
		( h->alias ) ? free( h->alias ) : 0 ;
		( h->dir ) ? free( h->dir ) : 0;
		( h->filter ) ? free( h->filter ) : 0 ;
		( h->root_default ) ? free( h->root_default ) : 0;
		free( h );
		hosts++;
	}
	free( config->hosts );
	free( config );
}


//Check that the website's chosen directory is accessible and it's log directory is writeable.
int check_dir ( struct host *host, char *err, int errlen ) {
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


//Generate a response via one of the selected filters
int pctx ( int fd, struct HTTPBody *req, struct HTTPBody *res, void *p ) {
	FPRINTF( "Proc started...\n" );
	char err[2048] = {0};
	Table *t = NULL;
	struct host *host = NULL;
	struct config *config = NULL;
	struct filter *filter = NULL;

	//With no default host, throw this 
	if ( !req->host ) {
		char *e = "No host header specified.";
		FPRINTF( e );
		return http_set_error( res, 500, err ); 
	}

	if (( config = build_config( err, sizeof(err) )) == NULL ) {
		return http_set_error( res, 500, err );
	}  

	if ( !( host = find_host( config->hosts, req->host ) ) ) {
		snprintf( err, sizeof(err), "Could not find host '%s'.", req->host );
		return http_set_error( res, 404, err ); 
	}

	if ( !( filter = check_filter( filters, !host->filter ? "static" : host->filter ) ) ) {
		snprintf( err, sizeof( err ), "Filter '%s' not supported", host->filter );
		return http_set_error( res, 500, err ); 
	}

	if ( host->dir && !check_dir( host, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err ); 
	}

	FPRINTF( "Root default of host '%s' is: %s\n", host->name, host->root_default );
	//Finally, now we can evalute the filter and the route.
	if ( !filter->filter( req, res, host ) ) {
		return 0;
	}

	//print_httpbody( res );	
	//The thing needs to be freed 
	free_config( config );

#if 0
	char *msg = strdup( "All is well." );
	http_set_status( res, 200 );
	http_set_ctype( res, strdup( "text/html" ));
	http_set_content_text( res, msg );
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		fprintf( stderr, "[%s:%d] %s\n", __FILE__, __LINE__, err );
		http_set_error( res, 500, err ); 
		return 0;
	}
#endif
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


//Loop should be extracted out
int accept_loop1( struct sockAbstr *su, char *err, int errlen ) {

	//Initialize
	struct senderrecvr *ctx = &sr[ CTXTYPE ];	
	ctx->init( &ctx->data );
	
	//This can have one global variable
	for ( ;; ) {
		//Client address and length?
		int fd, status;	
		pid_t cpid; 

		//Accept
		status = ctx->accept( su, &fd, ctx->data, err, errlen );

		//SSL problems need to be caught here
		if ( status == AC_EAGAIN || status == AC_EMFILE || status == AC_EEINTR ) {
			continue;
		}
		else if ( !status ) {
			FPRINTF( "accept() failed: %s\n", err );
			return 0;
		}

		//close the connection if something fails here
		if ( !srv_setsocketoptions( fd ) ) {
			FPRINTF( "socket %d could not be marked as non-blocking\n", fd );
			return 0;
		}

		//If something bad happens when writing logs, what could it be?
		if ( !srv_writelog( fd, su ) ) {
			continue;
		}

		//Fork and serve a request 
		if ( ( cpid = fork() ) == -1 ) {
			//TODO: There is most likely a reason this didn't work.
			FPRINTF( "Failed to setup new child connection. %s\n", strerror(errno) );
			return 0;
		}
		else if ( cpid == 0 ) {
			//TODO: Additional logging ought to happen here.
			FPRINTF( "In parent...\n" );
			//Close the file descriptor here?
			if ( close( fd ) == -1 ) {
				FPRINTF( "Parent couldn't close socket.\n" );
			}
		}
		else if ( cpid ) {
			struct HTTPBody rq = {0}, rs = {0};
			int status = 0;

			//Do any per-request initialization here.
			//ctx->pre( fd, &rq, &rs, ctx );

			//Read the message
			ctx->read( fd, &rq, &rs, ctx->data );
			print_httpbody( &rq ); //Dump the request 

			//Generate a message	
			ctx->proc && ctx->proc( fd, &rq, &rs, ctx->data ); 

			//Write a new message	
			ctx->write( fd, &rq, &rs, ctx->data );
			print_httpbody( &rs ); //Dump the request 

			//Close out
			ctx->free( fd, &rq, &rs, ctx->data );
			//ctx->post( fd, &rq, &rs, ctx );
			//_exit( 0 );
			return 1; 
		}
	}

	//Destroy anything that should have been long running.
	//ctx->free( &ctx->data );
	return 1;
}


int help () {
	const char *fmt = "  --%-10s       %-30s\n";
	fprintf( stderr, "%s: No options received.\n", __FILE__ );
	fprintf( stderr, fmt, "start", "start new servers" );
	fprintf( stderr, fmt, "debug", "set debug rules" );
	fprintf( stderr, fmt, "kill", "test killing a server" );
	fprintf( stderr, fmt, "fork", "daemonize the server" );
	fprintf( stderr, fmt, "port <arg>", "set a differnt port" );
	fprintf( stderr, fmt, "ssl", "use ssl or not.." );
	fprintf( stderr, fmt, "user <arg>", "choose a user to start as" );
	return 0;	
}


void print_options ( struct values *v ) {
	const char *fmt = "%-10s: %s\n";
	fprintf( stderr, "%s invoked with options:\n", __FILE__ );
	fprintf( stderr, "%10s: %d\n", "start", v->start );	
	fprintf( stderr, "%10s: %d\n", "kill", v->kill );	
	fprintf( stderr, "%10s: %d\n", "port", v->port );	
	fprintf( stderr, "%10s: %d\n", "fork", v->fork );	
	fprintf( stderr, "%10s: %s\n", "user", v->user );	
	fprintf( stderr, "%10s: %s\n", "ssl", v->ssl ? "true" : "false" );	
}


//We can also choose to daemonize the server or not.
//But is this too complicated?
#if 0
int daemonizer() {
	//If I could open a socket and listen successfully, then write the PID
	#if 0
	if ( values.fork ) {
		pid_t pid = fork();
		if ( pid == -1 ) {
			fprintf( stderr, "Failed to daemonize server process: %s", strerror(errno) );
			return 0;
		}
		else if ( !pid ) {
			//Close the parent?
			return 0;
		}
		else if ( pid ) {
			int len, fd = 0;
			char buf[64] = { 0 };

			if ( (fd = open( pidfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR )) == -1 )
				return ERRL( "Failed to access PID file: %s.", strerror(errno));

			len = snprintf( buf, 63, "%d", pid );

			//Write the pid down
			if (write( fd, buf, len ) == -1)
				return ERRL( "Failed to log PID: %s.", strerror(errno));
		
			//The parent exited successfully.
			if ( close(fd) == -1 )
				return ERRL( "Could not close parent socket: %s", strerror(errno));
			return SUC_PARENT;
		}
	}
	#endif
}
#endif

int main (int argc, char *argv[]) {
	struct values values = { 0 };
	char err[ 2048 ] = { 0 };

	if ( argc < 2 ) {
		return help();
	}
	
	while ( *argv ) {
		if ( strcmp( *argv, "--start" ) == 0 ) 
			values.start = 1;
		else if ( strcmp( *argv, "--kill" ) == 0 ) 
			values.kill = 1;
		else if ( strcmp( *argv, "--ssl" ) == 0 ) 
			values.ssl = 1;
		else if ( strcmp( *argv, "--daemonize" ) == 0 ) 
			values.fork = 1;
		else if ( strcmp( *argv, "--debug" ) == 0 ) 
			arg_debug = 1;
		else if ( strcmp( *argv, "--port" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				fprintf( stderr, "Expected argument for --port!" );
				return 0;
			}
			//TODO: This should be safeatoi 
			values.port = atoi( *argv );
		}
		else if ( strcmp( *argv, "--user" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				fprintf( stderr, "Expected argument for --user!" );
				return 0;
			} 
			values.user = strdup( *argv );
		}
		argv++;
	}

	if ( arg_debug ) {
		print_options( &values );
	}

	//Set all of the socket stuff
	struct sockAbstr su;
	int *port = !values.port ? (int *)&defport : &values.port;
	populate_tcp_socket( &su, port );

	if ( arg_debug ) {
		print_socket( &su ); 
	}

	if ( !open_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}

	//What kinds of problems can occur here?
	if ( !accept_loop1( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "Server failed. Error: %s\n", err );
		return 1;
	}

	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "Couldn't close parent socket. Error: %s\n", err );
		return 1;
	}

	return 0;
}


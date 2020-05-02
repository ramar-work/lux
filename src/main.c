/* -------------------------------------------------------- *
main.c
======

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
#include "../vendor/single.h"
#include "server.h"
#include "ctx-http.h"
#include "ctx-https.h"
#include "server.h"
#include "socket.h"
#include "filter-static.h"
#if 0
#include "filter-dirent.h"
#include "filter-echo.h"
#include "filter-lua.h"
#include "filter-c.h"
#endif


#define CTXTYPE 0

#define eprintf(...) fprintf( stderr, "%s: ", "hypno" ) && fprintf( stderr, __VA_ARGS__ ) && fprintf( stderr, "\n" )

struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
	char *config;
};

const int defport = 2000;
int arg_verbose = 0;
int arg_debug = 0;

struct filter filters[] = {
	{ "static", filter_static }
#if 0
, { "dirent", filter_dirent }
, { "echo", filter_echo }
, { "lua", filter_lua }
, { "c", filter_c }
#endif
, { NULL }
};

struct senderrecvr sr[] = {
	{ read_notls, write_notls, create_notls }
, { read_gnutls, write_gnutls, create_gnutls, NULL, pre_gnutls }
//, { NULL, read_static, write_static, free_notls  }
,	{ NULL }
};

//Loop should be extracted out
int cmd_server ( struct values *v, char *err, int errlen ) {
	//Define
	struct sockAbstr su;
	struct senderrecvr *ctx = NULL;

	//Initialize
	populate_tcp_socket( &su, &v->port );
	ctx = &sr[ CTXTYPE ];	
	ctx->init( &ctx->data );
	ctx->filters = filters;

	//Open a socket		
	if ( !open_listening_socket( &su, err, errlen ) ) {
		eprintf( "%s", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}

	//This can have one global variable
	for ( ;; ) {
		//Client address and length?
		int fd = 0, status;	
		pid_t cpid; 

		//Accept a connection.
		if ( !accept_listening_socket( &su, &fd, err, errlen ) ) {
			FPRINTF( "socket %d could not be marked as non-blocking\n", fd );
			continue;
		}

		//close the connection if something fails here
		if ( !srv_setsocketoptions( fd ) ) {
			FPRINTF( "socket %d could not be marked as non-blocking\n", fd );
			return 0;
		}

		//If something bad happens when writing logs, what could it be?
		if ( !srv_writelog( fd, &su ) ) {
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
			if ( !srv_response( fd, ctx ) ) {
				FPRINTF( "Error in TCP socket handling.\n" );
			}
		}
	}

	//Destroy anything that should have been long running.
	//ctx->free( &ctx->data );

	//Close the socket
	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		eprintf( "Couldn't close parent socket. Error: %s", err );
		return 0;
	}
	
	return 1;
}


int help () {
	const char *fmt = "  --%-10s       %-30s\n";
	fprintf( stderr, "%s: No options received.\n", __FILE__ );
	fprintf( stderr, fmt, "start", "start new servers" );
	fprintf( stderr, fmt, "debug", "set debug rules" );
	fprintf( stderr, fmt, "kill", "test killing a server" );
	fprintf( stderr, fmt, "fork", "daemonize the server" );
	fprintf( stderr, fmt, "config <arg>", "use this file for configuration" );
	fprintf( stderr, fmt, "port <arg>", "set a differnt port" );
	fprintf( stderr, fmt, "ssl", "use ssl or not.." );
	fprintf( stderr, fmt, "user <arg>", "choose a user to start as" );
#if 0
	fprintf( stderr, fmt, "dir <arg>", "Use this directory to serve from" );
#endif
	return 0;	
}


void print_options ( struct values *v ) {
	const char *fmt = "%-10s: %s\n";
	fprintf( stderr, "%s invoked with options:\n", __FILE__ );
	fprintf( stderr, "%10s: %d\n", "start", v->start );	
	fprintf( stderr, "%10s: %d\n", "kill", v->kill );	
	fprintf( stderr, "%10s: %d\n", "port", v->port );	
	fprintf( stderr, "%10s: %d\n", "fork", v->fork );	
	fprintf( stderr, "%10s: %s\n", "config", v->config );	
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
	int *port = NULL; 

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
		else if ( strcmp( *argv, "--config" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				eprintf( "Expected argument for --config!" );
				return 0;
			}
			values.config = *argv;
		}
		else if ( strcmp( *argv, "--port" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				eprintf( "Expected argument for --port!" );
				return 0;
			}
			//TODO: This should be safeatoi 
			values.port = atoi( *argv );
		}
		else if ( strcmp( *argv, "--user" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				eprintf( "Expected argument for --user!" );
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
	if ( !values.port )
		port = (int *)&defport;
	else {
		port = &values.port;
	}

	//Pull in a configuration
	if ( !values.config ) {
		eprintf( "No configuration specified." );
		return 1;
	}

	//Start a server
	if ( values.start ) {
	#if 0
		if ( values.fork ) {
			//start a fork...
		}
	#endif
		if ( !cmd_server( &values, err, sizeof(err) ) ) {
			eprintf( "%s", err );
			return 1;
		}
	}

	if ( values.kill ) {

	}

	return 0;
}


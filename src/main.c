/* -------------------------------------------------------- *
 * main.c
 * ======
 * 
 * Summary 
 * -------
 * This is the basis of hypno's web server.
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
 * 
 * Building
 * --------
 * Lua is a necessity because of the configuration parsing. 
 * 
 * Running make is all we need for now on Linux and OSX.  Windows
 * will need some additional help and Cygwin as well.
 * 
 * -------------------------------------------------------- */
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "ctx-http.h"
#include "ctx-https.h"
#include "server.h"
#include "socket.h"
#include "filter-static.h"
#include "filter-lua.h"
#if 0
#include "filter-dirent.h"
#include "filter-echo.h"
#include "filter-c.h"
#endif

#define eprintf(...) fprintf( stderr, "%s: ", "hypno" ) && fprintf( stderr, __VA_ARGS__ ) && fprintf( stderr, "\n" )

const int defport = 2000;

int arg_verbose = 0;

int arg_debug = 0;

struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
	char *config;
};


//Deifne a list of filters
struct filter filters[] = {
	{ "static", filter_static },
#if 0
  { "lua", filter_lua },
, { "dirent", filter_dirent }
, { "echo", filter_echo }
, { "c", filter_c }
#endif
  { NULL }
};


//In lieu of an actual ctx object, we do this to mock pre & post which don't exist
const 
int fkctpre( int fd, struct HTTPBody *a, struct HTTPBody *b, struct cdata *c ) {
	return 1;
}

const 
int fkctpost( int fd, struct HTTPBody *a, struct HTTPBody *b, struct cdata *c) {
	return 1;
}


//Define a list of "context types"
struct senderrecvr sr[] = {
	{ read_notls, write_notls, create_notls, NULL, fkctpre, fkctpost  }
, { read_gnutls, write_gnutls, create_gnutls, NULL, pre_gnutls, post_gnutls }
,	{ NULL }
};


//Loop should be extracted out
int cmd_server ( struct values *v, char *err, int errlen ) {
	//Define
	struct sockAbstr su = {0};
	struct senderrecvr *ctx = NULL;

	//Initialize
	populate_tcp_socket( &su, &v->port );
	ctx = &sr[ v->ssl ];	
	ctx->init( &ctx->data );
	ctx->filters = filters;
	ctx->config = v->config;

	//Die if config is null or file not there 
	if ( !ctx->config ) {
		eprintf( "No config specified...\n" );
		return 0;
	}

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

	#if 0
		//close the connection if something fails here
		if ( !srv_setsocketoptions( fd ) ) {
			FPRINTF( "socket %d could not be marked as non-blocking\n", fd );
			return 0;
		}
	#endif

	#if 1
	#if 0
		if ( fcntl( fd, F_SETFD, SIGIO ) == -1 ) {
			FPRINTF( "Error in setting SIGIO on filedes %d.\n", fd );
			return 0;
		}

		if ( fcntl( fd, F_SETFL, SIGIO ) == -1 ) {
			FPRINTF( "Error in setting SIGIO on filedes %d.\n", fd );
			return 0;
		}
	#endif
	#endif

	#ifdef NOFORK_H
		struct cdata connection = {0};	
		connection.flags = O_NONBLOCK;
		connection.ctx = ctx;
	
		//Get IP here and save it for logging purposes
		if ( !get_iip_of_socket( &su ) || !( connection.ipv4 = su.iip ) ) {
			FPRINTF( "Error in getting IP address of connecting client.\n" );
		}

		for ( ;; ) {
			//additionally, this one should block
			if ( !srv_response( fd, &connection ) ) {
				FPRINTF( "Error in TCP socket handling.\n" );
			}

			if ( connection.count < 0 || connection.count > 5 ) {
				FPRINTF( "Closing connection marked by descriptor %d to peer.\n", fd );
				int status = close( fd );
				if ( status == -1 ) {
					FPRINTF( "Error when closing child socket.\n" );
				}
				break;
			}

			FPRINTF( "Connection is done. count is %d\n", connection.count );
		}
	#endif

	#ifdef FORK_H
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
			struct cdata connection = {0};	
			connection.flags = O_NONBLOCK;
			connection.ctx = ctx;
		
			//Get IP here and save it for logging purposes
			if ( !get_iip_of_socket( &su ) || !( connection.ipv4 = su.iip ) ) {
				FPRINTF( "Error in getting IP address of connecting client.\n" );
			}

			for ( ;; ) {
				//additionally, this one should block
				if ( !srv_response( fd, &connection ) ) {
					FPRINTF( "Error in TCP socket handling.\n" );
				}

				if ( connection.count < 0 || connection.count > 5 ) {
					FPRINTF( "Closing connection marked by descriptor %d to peer.\n", fd );
					if ( close( fd ) == -1 ) {
						FPRINTF( "Error when closing child socket.\n" );
					}
					break;
				}

				FPRINTF( "Connection is done. count is %d\n", connection.count );
			}
		}
	#endif
	}

	//Close the socket
	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		eprintf( "Couldn't close parent socket. Error: %s", err );
		return 0;
	}
	
	return 1;
}


//Display help
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


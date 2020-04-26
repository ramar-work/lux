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
#include "ssl.h"
#include "filter-static.h"
#include "filter-dirent.h"
#include "filter-echo.h"
#include "filter-lua.h"
#include "filter-c.h"


int srv_fork ( int fd );
int srv_thread ( int fd );
int srv_vanilla ( int fd );
int srv_test ( int fd );
int srv_dummy ( int *times );
int srv_inccount( int *times );
int h_read ( int, struct HTTPBody *, struct HTTPBody *, void * );
int h_proc ( int, struct HTTPBody *, struct HTTPBody *, void * );
int h_write( int, struct HTTPBody *, struct HTTPBody *, void * );
int t_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx );
int t_write( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx );


struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * );
};

struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void *readf;
	void *writef;
}; 

struct model {
	int (*exec)( int );
	int (*stop)( int * );
	void *data;
};

struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
}; 

struct model models[] = {
	{ srv_fork, srv_dummy },	
	{ srv_thread, srv_dummy },	
	{ srv_vanilla, srv_dummy },	
	{ srv_test, srv_inccount },	
};


struct senderrecvr sr[] = {
	{ h_read, h_proc, h_write }
, { t_read, NULL, t_write  }
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

const int AC_EAGAIN = 2;
const int AC_EMFILE = 3;
const int AC_EEINTR = 4;

const int defport = 2000;
int arg_verbose = 0;
int arg_debug = 0;


//send (sends a message, may take many times)
int t_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//write (write all the data in one call if you fork like this) 
	const char http_200[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";
	if ( write( fd, http_200, strlen(http_200)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
int t_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message...\n" );
		close(fd);
		return 0;
	}

	return 0;
}


int h_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *sess ) {
	//Read all the data from a socket.
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	
	char err[ 2048 ] = {0};

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//Read functions...
		if ( !sess ) {
			rd = recv( fd, buf2, size, MSG_DONTWAIT );
		}
		else {
			gnutls_session_t *ss = (gnutls_session_t *)sess;
			if ( ( rd = gnutls_record_recv( *ss, buf2, size ) ) > 0 ) {
				fprintf(stderr, "SSL might be fine... got %d from gnutls_record_recv\n", rd );
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				fprintf(stderr, "SSL got handshake reauth request..." );
				//TODO: There should be a seperate function that handles this.
				//It's a fail for now...	
				return 0;
			}
			else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				fprintf(stderr, "SSL was interrupted...  Try request again...\n" );
				continue;
			}
			else {
				fprintf(stderr, "SSL got error code: %d, meaning '%s'.\n", rd, gnutls_strerror( rd ) );
				continue;
			}
		}

		//Read into a static buffer
		if ( rd == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message...\n" );
			whatsockerr( errno );
#if 0
			if ( 0 ) {
				0;
			}
			//ssl stuff has to go first...
			else if ( errno == EBADF ) {
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			}
			else if ( errno == ECONNREFUSED ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == EFAULT ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == EINTR ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == EINVAL ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == ENOMEM ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == ENOTCONN ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else if ( errno == ENOTSOCK ) {
				return http_set_error( rs, 500, strerror( errno ) );
			}
			else 
#endif
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "rq->mlen: %d\n", rq->mlen );
					fprintf(stderr, "%p\n", buf );
					//rq->msg = buf;
					break;
				}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
				return http_set_error( rs, 500, strerror( errno ) );
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			rq->msg = buf;
			break;
		}
	#if 0
		else if ( rd == GNUTLS_E_REHANDSHAKE ) {
			fprintf(stderr, "SSL got handshake reauth request..." );
			continue;
		}
		else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
			fprintf(stderr, "SSL was interrupted...  Try request again...\n" );
			continue;
		}
	#endif
		else {
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
#if 0
			fprintf(stderr, "buf: %p\n", buf );
			fprintf(stderr, "buf2: %p\n", buf2 );
			fprintf(stderr, "pos: %d\n", bfsize - size );
#endif
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			FPRINTF( "Recvd %d bytes on fd %d\n", rd, fd ); 
		}
	}

	if ( !http_parse_request( rq, err, sizeof(err) ) ) {
		return http_set_error( rs, 500, err ); 
	}
	return 1;
}


//Write
int h_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//if ( 1 ) write( 2, rq->msg, rq->mlen );
	int sent = 0;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

	while ( 1 ) { 
		FPRINTF( "Attempting to send %d bytes.", total );
		if (( sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT )) == -1 ) {
			if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED ) {
				FPRINTF( "Connection refused." );
				return -1;//close(fd);
			}
			else if ( errno == EFAULT ) {
				FPRINTF( "EFAULT." );
				return -1;//close(fd);
			}
			else if ( errno == EINTR ) {
				FPRINTF( "EINTR." );
				return -1;//close(fd);
			}
			else if ( errno == EINVAL ) {
				FPRINTF( "EINVAL." );
				return -1;//close(fd);
			}
			else if ( errno == ENOMEM ) {
				FPRINTF( "Out of memory." );
				return -1;//close(fd);
			}
			else if ( errno == ENOTCONN ) {
				return -1;//close(fd);
			}
			else if ( errno == ENOTSOCK ) {
				return -1;//close(fd);
			}
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "rs->mlen: %d\n", rs->mlen );
					//rq->msg = buf;
					break;
				}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
			}
		}
		else if ( total == 0 ) {
			break;
		}
		else if ( sent == 0 ) {
		}
		else {
			//continue resending...
			pos += sent;
			total -= sent;	
		}
	}
	return 1;
}


struct host * find_host ( struct host **hosts, char *hostname ) {
	char host[ 2048 ] = { 0 };
	int pos = memchrat( hostname, ':', strlen( hostname ) );
	memcpy( host, hostname, ( pos > -1 ) ? pos : strlen(hostname) );
FPRINTF( "selected hostname: %s\n", host );
	while ( hosts && *hosts ) {
		struct host *req = *hosts;
FPRINTF( "hostname: %s\n", req->name );
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


//Need to throw lots in here...
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


int h_proc ( int fd, struct HTTPBody *req, struct HTTPBody *res, void *ctx ) {
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
	return 1;
}



int ssl_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//write (write all the data in one call if you fork like this) 
	if ( write( fd, http_200_fixed, strlen(http_200_fixed)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
int ssl_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message...\n" );
		close(fd);
		return 0;
	}

	return 0;
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


int srv_accept( int parent, struct sockaddr *addrinfo, socklen_t *addrlen, int *child, char *err, int errlen ) {
	
	//Accept a connection if possible...
	if (( *child = accept( parent, addrinfo, addrlen )) == -1 ) {
		//TODO: Need to check if the socket was non-blocking or not...
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			//This should just try to read again
			FPRINTF( "Try accept again.\n" );
			return AC_EAGAIN;	
		}
		else if ( errno == EMFILE || errno == ENFILE ) { 
			//These both refer to open file limits
			FPRINTF( "Too many open files, try closing some requests.\n" );
			return AC_EMFILE;	
		}
		else if ( errno == EINTR ) { 
			//In this situation we'll handle signals
			FPRINTF( "Signal received. (Not coded yet.)\n" );
			return AC_EEINTR;	
		}
		else {
			//All other codes really should just stop. 
			snprintf( err, errlen, "accept() failed: %s\n", strerror( errno ) );
			return 0;
		}
	}
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
int srv_writelog ( int fd, struct sockaddr *addrinfo ) {
	struct sockaddr_in *cin = (struct sockaddr_in *)addrinfo;
	char *ip = inet_ntoa( cin->sin_addr );
	fprintf( stderr, "Got request from: %s on new file: %d\n", ip, fd );	
	return 1;
}



//Handles generating a request
int srv_generaterequest ( int fd ) {
	//TODO: Building config may need to take place here...
	struct HTTPBody rq, rs;
	struct senderrecvr *f = &sr[ 0 ]; 
	int status = 0;
	memset( &rq, 0, sizeof( struct HTTPBody ) );
	memset( &rs, 0, sizeof( struct HTTPBody ) );
#if 1
	void *session = NULL;
#else
	//TODO: This doesn't seem quite optimal, but I'm doing it.
	struct SSLContext ssl = {
		.read = gnutls_record_recv
	, .write = gnutls_record_send
	, .data = (void *)&rq
	}; 
#endif

	//Read the message
	if (( status = f->read( fd, &rq, &rs, session )) == -1 ) {
	}
	FPRINTF( "Read complete.\n" );

	//Dump the body?
	print_httpbody( &rq );

	//Generate a message	
	if ( f->proc && ( status = f->proc( fd, &rq, &rs, NULL )) == -1 ) {
	}
	FPRINTF( "Proc complete.\n" );

	//Write a new message	
	if (( status = f->write( fd, &rq, &rs, session )) == -1 ) {
	}
	FPRINTF( "Write complete.\n" );

	//Free this and close the file
	http_free_body( &rs );
	http_free_body( &rq );

	if ( close( fd ) == -1 ) {
		fprintf( stderr, "Couldn't close child socket. %s\n", strerror(errno) );
		return 1;
	}

	FPRINTF( "Deallocation complete.\n" );
	return 1;
}


int srv_dummy ( int *times ) {
	return 0;
}

int srv_inccount( int *times ) {
	(*times) += 1;
	return ( *times > 1 );
}


//Use forks
int srv_fork ( int fd ) {
	//Fork and go crazy
	pid_t cpid = fork();
	if ( cpid == -1 ) {
		//TODO: There is most likely a reason this didn't work.
		fprintf( stderr, "Failed to setup new child connection. %s\n", strerror(errno) );
		return 0;
	}
	else if ( cpid == 0 ) {
		//TODO: The parent should probably log some important info here.	
		fprintf(stderr, "in parent...\n" );
		//Close the file descriptor here?
		if ( close( fd ) == -1 ) {
			fprintf( stderr, "Parent couldn't close socket." );
		}
	}
	else if ( cpid ) {
		srv_generaterequest( fd );
		if ( close( fd ) == -1 ) {
			fprintf( stderr, "Child couldn't close its socket." );
		}
	}
	return 1;
}


//Use threads
int srv_thread ( int fd ) {
	return 0;
}


//Use no means of concurrency
int srv_vanilla ( int fd ) {
	srv_generaterequest( fd );
	return 1;
}


//Use no means of concurrency and assume that the process is done after one iteration
int srv_test( int fd ) {
	srv_generaterequest( fd );
	return 1;
}


//Loop should be extracted out
int accept_loop1( struct sockAbstr *su, struct model *srv_type, char *err, int errlen ) {

	//This can have one global variable that controls how long the loop runs...
	int times = 0;
	for ( ; !srv_type->stop( &times ) ; ) {
		//Client address and length?
		struct sockaddr addrinfo;	
		socklen_t addrlen = sizeof (struct sockaddr);	
		int fd, status;	
		
		status = srv_accept( su->fd, &addrinfo, &addrlen, &fd, err, errlen );
		if ( status == AC_EAGAIN || status == AC_EMFILE || status == AC_EEINTR ) {
			continue;
		}
		else if ( !status ) {
			FPRINTF( "accept() failed: %s\n", err );
			return 0;
		}

		if ( !srv_setsocketoptions( fd ) ) {
			//close the connection if something fails here
			continue;
		}

		if ( !srv_writelog( fd, &addrinfo ) ) {
			//something else happened, but it's not fatal...
			continue;
		}

		if ( !srv_type->exec( fd ) ) {
			//This is not technically a failure either...
		}
	}
	return 1;
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

	//Handle SSL here
	#if 0
	struct gnutls_abstr g;
	open_ssl_context( &g );
	//Initialize SSL?
	#endif

	//What kinds of problems can occur here?
	if ( !accept_loop1( &su, &models[3], err, sizeof(err) ) ) {
		fprintf( stderr, "Server failed. Error: %s\n", err );
		return 1;
	}

	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "Couldn't close parent socket. Error: %s\n", err );
		return 1;
	}

	return 0;
}


/*let's try this again.  It seems never to work like it should...*/
#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luabind.h"
#include "http.h"
#include "socket.h"
#include "util.h"
#include "ssl.h"

#include "filter-static.h"

extern const char http_200_custom[];
extern const char http_200_fixed[];
extern const char http_400_custom[];
extern const char http_400_fixed[];
extern const char http_500_custom[];
extern const char http_500_fixed[];

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

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//Read functions...
		if ( !sess )
			rd = recv( fd, buf2, size, MSG_DONTWAIT );
		else {
			gnutls_session_t *ss = (gnutls_session_t *)sess;
			rd = gnutls_record_recv( *ss, buf2, size );
			if ( rd > 0 ) 
				fprintf(stderr, "SSL might be fine... got %d from gnutls_record_recv\n", rd );
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

		//read into new buffer
		//TODO: Yay!  This works great on Arch!  But let's see what about Win, OSX and BSD
		if ( rd == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message...\n" );
			whatsockerr( errno );
			if ( 0 )
				0;
			//ssl stuff has to go first...
			else if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
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
				fprintf(stderr, "Couldn't allocate buffer..." );
				close(fd);
				return 0;
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
			fprintf(stderr, "buf: %p\n", buf );
			fprintf(stderr, "buf2: %p\n", buf2 );
			fprintf(stderr, "pos: %d\n", bfsize - size );
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			if ( 1 ) {
				fprintf( stderr, "Recvd %d bytes on fd %d\n", rd, fd ); 
			}
		}
	}

#if 1
	char err[ 2048 ] = {0};
	if ( !http_parse_response( rq, err, sizeof(err) ) ) {
		//close( fd );
		return 0;
	}
#else
	//Prepare the rest of the request
	char *header = (char *)rq->msg;
	int pLen = memchrat( rq->msg, '\n', rq->mlen ) - 1;
	const int flLen = pLen + strlen( "\r\n" );
	int hdLen = memstrat( rq->msg, "\r\n\r\n", rq->mlen );

	//Initialize the remainder of variables 
	rq->headers = NULL;
	rq->body = NULL;
	rq->url = NULL;
	rq->method = get_lstr( &header, ' ', &pLen );
	rq->path = get_lstr( &header, ' ', &pLen );
	rq->protocol = get_lstr( &header, ' ', &pLen ); 
	rq->hlen = hdLen; 
	rq->host = msg_get_value( "Host: ", "\r", rq->msg, hdLen );

	//The protocol parsing can happen here...
	if ( strcmp( rq->method, "HEAD" ) == 0 )
		;
	else if ( strcmp( rq->method, "GET" ) == 0 )
		;
	else if ( strcmp( rq->method, "POST" ) == 0 ) {
		rq->clen = safeatoi( msg_get_value( "Content-Length: ", "\r", rq->msg, hdLen ) );
		rq->ctype = msg_get_value( "Content-Type: ", ";\r", rq->msg, hdLen );
		rq->boundary = msg_get_value( "boundary=", "\r", rq->msg, hdLen );
		//rq->mlen = hdLen; 
		//If clen is -1, ... hmmm.  At some point, I still need to do the rest of the work. 
	}

	if ( 0 )  {
		print_httpbody( rq );	
	}

	//Define records for each type here...
	//struct HTTPRecord **url=NULL, **headers=NULL, **body=NULL;
	int len = 0;
	Mem set;
	memset( &set, 0, sizeof( Mem ) );

	//Always process the URL (specifically GET vars)
	if ( strlen( rq->path ) == 1 ) {
		ADDITEM( NULL, struct HTTPRecord, rq->url, len, 0 );
	}
	else {
		int index = 0;
		while ( strwalk( &set, rq->path, "?&" ) ) {
			uint8_t *t = (uint8_t *)&rq->path[ set.pos ];
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			memset( b, 0, sizeof( struct HTTPRecord ) );
			int at = memchrat( t, '=', set.size );
			if ( !b || at == -1 || !set.size ) 
				;
			else {
				int klen = at;
				b->field = copystr( t, klen );
				klen += 1, t += klen, set.size -= klen;
				b->value = t;
				b->size = set.size;
				ADDITEM( b, struct HTTPRecord, rq->url, len, 0 );
			}
		}
		ADDITEM( NULL, struct HTTPRecord, rq->url, len, 0 );
	}


	//Always process the headers
	memset( &set, 0, sizeof( Mem ) );
	len = 0;
	uint8_t *h = &rq->msg[ flLen - 1 ];
	while ( memwalk( &set, h, (uint8_t *)"\r", rq->hlen, 1 ) ) {
		//Break on newline, and extract the _first_ ':'
		uint8_t *t = &h[ set.pos - 1 ];
		if ( *t == '\r' ) {  
			int at = memchrat( t, ':', set.size );
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			memset( b, 0, sizeof( struct HTTPRecord ) );
			if ( !b || at == -1 || at > 127 )
				;
			else {
				at -= 2, t += 2;
				b->field = copystr( t, at );
				at += 2 /*Increment to get past ': '*/, t += at, set.size -= at;
				b->value = t;
				b->size = set.size - 1;
			#if 0
				DUMP_RIGHT( b->value, b->size );
			#endif
				ADDITEM( b, struct HTTPRecord, rq->headers, len, 0 );
			}
		}
	}
	ADDITEM( NULL, struct HTTPRecord, rq->headers, len, 0 );

	//Always process the body 
	memset( &set, 0, sizeof( Mem ) );
	len = 0;
	uint8_t *p = &rq->msg[ rq->hlen + strlen( "\r\n" ) ];
	int plen = rq->mlen - rq->hlen;
	
	//TODO: If this is a xfer-encoding chunked msg, rq->clen needs to get filled in when done.
	if ( strcmp( "POST", rq->method ) != 0 ) {
		ADDITEM( NULL, struct HTTPRecord, rq->body, len, 0 );
	}
	else {
		struct HTTPRecord *b = NULL;
		#if 0
		DUMP_RIGHT( p, rq->mlen - rq->hlen ); 
		#endif
		//TODO: Bitmasking is 1% more efficient, go for it.
		int name=0, value=0, index=0;

		//url encoded is a little bit different.  no real reason to use the same code...
		if ( strcmp( rq->ctype, "application/x-www-form-urlencoded" ) == 0 ) {
			//NOTE: clen is up by two to assist our little tokenizer...
			while ( memwalk( &set, p, (uint8_t *)"\n=&", rq->clen + 2, 3 ) ) {
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( *m == '\n' || *m == '&' ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
					//TODO: Should be checking that allocation was successful
					b->field = copystr( ++m, set.size );
				}
				else if ( *m == '=' ) {
					b->value = ++m;
					b->size = set.size;
					ADDITEM( b, struct HTTPRecord, rq->body, len, 0 );
					b = NULL;
				}
			}
		}
		else {
			while ( memwalk( &set, p, (uint8_t *)"\r:=;", rq->clen, 4 ) ) {
				//TODO: If we're being technical, set.pos - 1 can point to a negative index.  
				//However, as long as headers were sent (and 99.99999999% of the time they will be)
				//this negative index will point to valid allocated memory...
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( memcmp( m, "; name=", 7 ) == 0 )
					name = 1;
				//"\r\n\r\n"
				else if ( memcmp( m, "\r\n\r\n", 4 ) == 0 && !value )
					value = 1;
				else if ( memcmp( m, "\r\n-", 3 ) == 0 && !value ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
				}
				else if ( memcmp( m, "\r\n", 2 ) == 0 && value == 1 ) {
					m += 2;
					b->value = m;//++t;
					b->size = set.size - 1;
					ADDITEM( b, struct HTTPRecord, rq->body, len, 0 );
					value = 0;
					b = NULL;
				}
				else if ( *m == '=' && name == 1 ) {
					//fprintf( stderr, "copying name field... pass %d\n", ++index );
					int size = *(m + 1) == '"' ? set.size - 2 : set.size;
				#if 1
					m += ( *(m + 1) == '"' ) ? 2 : 1 ;
				#else
					int ptrinc = *(m + 1) == '"' ? 2 : 1;
					m += ptrinc;
				#endif
					b->field = copystr( m, size );
					name = 0;
				}
			}
		}

		//Add a terminator element
		ADDITEM( NULL, struct HTTPRecord, rq->body, len, 0 );
		//This MAY help in handling malformed messages...
		( b && (!b->field || !b->value) ) ? free( b ) : 0;

		if ( 0 ) {
			fprintf( stderr, "BODY got:\n" );
			print_httprecords( rq->body );
		}
	}

	//for testing, this should stay here...
	//close(fd);
#endif
	return 0;
}


//Write
int h_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//if ( 1 ) write( 2, rq->msg, rq->mlen );
	int sent = 0;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

	while ( 1 ) { 
		if (( sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT )) == -1 ) {
			if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
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



int h_proc ( int fd, struct HTTPBody *req, struct HTTPBody *res, void *ctx ) {
	char err[2048] = {0};
	lua_State *L = luaL_newstate();
	Table *t = NULL;
	memset( req, 0, sizeof(struct HTTPBody) );

	//After this conversion takes place, destroy the environment
	if ( !lua_exec_file( L, "www/config.lua", err, sizeof(err) ) ) {
		return http_set_error( res, 500, err ); 
	}

	if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, sizeof(err), "%s\n", "Could not initialize memory at h_proc" );
		return http_set_error( res, 500, err ); 
	}

#if 1
	//Check that server supports host - send 404 if not
	//build_hosts_list	

	//Check filter - send 500 if not there or if not supported
	//?

	//Get the host's table... then 

	// - Check that log dir is writeable - send 500 if not

	// - Check that the requested dir is readable - send 500 if not

	// - Populate any data structures that may be needed
#else
	if ( req->host && strcmp( req->host, "machine.com" ) ) == 0 ) {
		
	}   
#endif

#if 0
	// Run the filter...
	// filter( req, res, userdata );
#else
	char *msg = strdup( "All is well." );
	http_set_status( res, 200 );
	http_set_ctype( res, strdup( "text/html" ));
	http_set_content_text( res, msg );
#endif
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		http_set_error( res, 500, err ); 
		return 0;
	}
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


int f_proc ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
return 0;
}


struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
}; 

struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void *readf;
	void *writef;
} sr[] = {
	{ h_read, h_proc, h_write }
, { t_read, NULL, t_write  }
,	{ NULL }
};



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



const char *fqdn[] = {
	"localhost",
	NULL
};


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
	su.addrsize = sizeof(struct sockaddr);
	su.buffersize = 1024;
	su.opened = 0;
	su.backlog = 500;
	su.waittime = 5000;
	su.protocol = IPPROTO_TCP; // || IPPROTO_UDP
	su.socktype = SOCK_STREAM; // || SOCK_DGRAM
	su.iptype = PF_INET;
	su.reuse = SO_REUSEADDR;
	su.port = !values.port ? (int *)&defport : &values.port;
	su.ssl_ctx = NULL;

	if ( arg_debug ) {
		print_socket( &su ); 
	}

#if 1
	if ( !open_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}
#else
	//Create the socket body
	struct sockaddr_in t;
	memset( &t, 0, sizeof( struct sockaddr_in ) );
	struct sockaddr_in *sa = &t;
	sa->sin_family = su.iptype; 
	sa->sin_port = htons( *su.port );
	(&sa->sin_addr)->s_addr = htonl( INADDR_ANY );

	//Open the socket (non-blocking, preferably)
	int status;
	if (( su.fd = socket( su.iptype, su.socktype, su.protocol )) == -1 ) {
		fprintf( stderr, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	#if 0
	//Set timeout, reusable bit and any other options 
	struct timespec to = { .tv_sec = 2 };
	if ( setsockopt(su.fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1 ) {
		// su.free(sock);
		su.err = errno;
		return (0, "Could not reopen socket.");
	}
	#endif
	if ( fcntl( su.fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		fprintf( stderr, "fcntl error: %s\n", strerror(errno) ); 
		return 0;
	}

	if (( status = bind( su.fd, (struct sockaddr *)&t, sizeof(struct sockaddr_in))) == -1 ) {
		fprintf( stderr, "Couldn't bind socket to address! Error: %s\n", strerror( errno ) );
		return 0;
	}

	if (( status = listen( su.fd, su.backlog) ) == -1 ) {
		fprintf( stderr, "Couldn't listen for connections! Error: %s\n", strerror( errno ) );
		return 0;
	}

	//Mark open flag.
	su.opened = 1;
#endif

	//Handle SSL here
	#if 0 
	gnutls_certificate_credentials_t x509_cred = NULL;
  gnutls_priority_t priority_cache;
	const char *cafile, *crlfile, *certfile, *keyfile;
	#if 0
	cafile = 
	crlfile = 
	#endif
	#if 0
	//These should always be loaded, and there will almost always be a series
	certfile = 
	keyfile = 
	#else
#define MPATH "/home/ramar/prj/hypno/certs/collinsdesign.net"
	//Hardcode these for now.
	certfile = MPATH "/collinsdesign_net.crt";
	keyfile = MPATH "/server.key";
	#endif
	//Obviously, this is great for debugging TLS errors...
	//gnutls_global_set_log_function( tls_log_func );
	gnutls_global_init();
	gnutls_certificate_allocate_credentials( &x509_cred );
	//find the certificate authority to use
	//gnutls_certificate_set_x509_trust_file( x509_cred, cafile, GNUTLS_X509_FMT_PEM );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	gnutls_certificate_set_x509_key_file( x509_cred, certfile, keyfile, GNUTLS_X509_FMT_PEM );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	gnutls_priority_init( &priority_cache, NULL, NULL );
	#endif

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


	//Let's start the accept loop here...
	for ( ;; ) {
		//Client address and length?
		struct sockaddr addrinfo;	
		socklen_t addrlen = sizeof (struct sockaddr);	
		int fd;	

		//Accept a connection if possible...
		if (( fd = accept( su.fd, &addrinfo, &addrlen )) == -1 ) {
			//TODO: Need to check if the socket was non-blocking or not...
			if ( 0 )
				; 
			else {
				fprintf( stderr, "Accept ran into trouble: %s\n", strerror( errno ) );
				continue;
			}
		}

	#if 0
		//Make the new socket non-blocking too...
		if ( fcntl( fd, F_SETFD, O_NONBLOCK ) == -1 ) {
			fprintf( stderr, "fcntl error at child socket: %s\n", strerror(errno) ); 
			return 0;
		}
	#endif

		//Dump the client info and the child fd
		if ( 1 ) {
			struct sockaddr_in *cin = (struct sockaddr_in *)&addrinfo;
			char *ip = inet_ntoa( cin->sin_addr );
			fprintf( stderr, "Got request from: %s on new file: %d\n", ip, fd );	
		}


		//Fork and go crazy
		pid_t cpid = fork();
		if ( cpid == -1 ) {
			//TODO: There is most likely a reason this didn't work.
			fprintf( stderr, "Failed to setup new child connection. %s\n", strerror(errno) );
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
			//TODO: Somewhere in here, a signal needs to run that allows this thing to die.
			//TODO: Handle read and write errno cases 
			#if 0
			//SSL again
			gnutls_session_t session, *sptr = NULL;
			if ( values.ssl ) {
				gnutls_init( &session, GNUTLS_SERVER );
				gnutls_priority_set( session, priority_cache );
				gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, x509_cred );
				//NOTE: I need to do this b/c clients aren't expected to send a certificate with their request
				gnutls_certificate_server_set_request( session, GNUTLS_CERT_IGNORE ); 
				gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
				//Bind the current file descriptor to GnuTLS instance.
				gnutls_transport_set_int( session, fd );
				//Do the handshake here
				//TODO: I write nothing that looks like this, please refactor it...
				int success = 0;
				do {
					success = gnutls_handshake( session );
				} while ( success == GNUTLS_E_AGAIN || success == GNUTLS_E_INTERRUPTED );	
				if ( success < 0 ) {
					close( fd );
					gnutls_deinit( session );
					//TODO: Log all handshake failures.  Still don't know where.
					fprintf( stderr, "%s\n", "SSL handshake failed." );
					continue;
				}
				fprintf( stderr, "%s\n", "SSL handshake successful." );
				sptr = &session;
			}
			#endif

			//Somewhere in here, I'll have to find routes, parse conf, etc
			//
			//What site is this asking for? (Host header)
			//Where is said site on a filesystem?

			//All the processing occurs here.
			struct HTTPBody rq = { 0 }; 
			struct HTTPBody rs = { 0 };
			struct senderrecvr *f = &sr[ 0 ]; 
			int status = 0;
		#if 0
			//TODO: This doesn't seem quite optimal, but I'm doing it.
			struct SSLContext ssl = {
				.read = gnutls_record_recv
			, .write = gnutls_record_send
			, .data = (void *)&rq
			}; 
		#endif
			//Read the message	
		#if 0
			if (( status = f->read( fd, &rq, &rs, sptr )) == -1 ) {
		#else
			if (( status = f->read( fd, &rq, &rs, NULL )) == -1 ) {
		#endif
				//what to do with the response...
			}

			//Generate a new message	
			if ( f->proc && ( status = f->proc( fd, &rq, &rs, NULL )) == -1 ) {
				//...
			}

//fprintf( stderr, "msg & mlen: " ); write( 2, rs.msg, rs.mlen ); getchar();

			//Write a new message	
		#if 0
			if (( status = f->write( fd, &rq, &rs, &session )) == -1 ) {
		#else
			if (( status = f->write( fd, &rq, &rs, NULL )) == -1 ) {
		#endif
				//...
			}

			if ( close( fd ) == -1 ) {
				fprintf( stderr, "Couldn't close child socket. %s\n", strerror(errno) );
				return 1;
			}
		}
	}

#if 1
	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1;
	}
#else
	//Close the server process.
	if ( close( su.fd ) == -1 ) {
		fprintf( stderr, "Couldn't close socket! Error: %s\n", strerror( errno ) );
		return 0;
	}
#endif

	return 0;
}


/*let's try this again.  It seems never to work like it should...



Recvr, HTTP_Request and HTTP_Response all work ok...
Why in the world does this code always break?
*/

struct HTTPBody {
	int clen;  //content length
	int mlen;  //message length (length of the entire received message)
	int	hlen;  //header length
	int status; //what was this?
#if 0	
	char      method[HTTP_METHOD_MAX];  //one of 7 methods
	char      protocol[HTTP_PROTO_MAX]; //
	char      path[HTTP_URL_MAX];       //The requested path
	char      host[1024];               //safe bet for host length
	char      boundary[128];            //The boundary
#endif
	char      *stext; //status text ptr
	char      *ctype; //content type ptr
	char      *method;
	char      *protocol;
	char      *path;
	char      *host;
	char      *boundary;
 	unsigned char *msg;

	//Simple data structures.  like headers on both sides...
	//Can't think if you need hash tables or not...
	//Table     table;
};



/*???*/
struct Loop {
//	Type type;	//what other type would there be?
	struct pollfd   *client;  //Pointer to currently being served client
	int bytes;  //bytes received or sent
	int *fd;  //???
	int retries;
  unsigned char *msg;
	void          *userdata;
	//struct timespec start, end;
	int      connNo; 
};



struct sockAbstr {
	int addrsize;
	int buffersize;
	int opened;
	int backlog;
	int waittime;
	int protocol;
	int socktype;
	int fd;
	int iptype; //ipv4 or v6
	int reuse;
	int family;
	int *port;
	struct sockaddr_in *sin;	
	void *ssl_ctx;
};

#include "vendor/single.h"
//#include "vendor/http.h"

//pre
int http_pre( ) {
	return 0;
}
//post (these are for ssl teardown and creation)
int http_post( ) {
	return 0;
}

//send (sends a message, may take many times)
//int http_send( ) {
int l_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
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
//int http_read( ) {
int l_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


#if 1


int main (int argc, char *argv[]) {

	struct values {
		int port;
		int ssl;
		int start;
		int kill;
		int fork;
		char *user;
	} values = { 0 };

	//Process all your options...
	//--start - start new servers
	//--kill - test killing a server
	//--port - set a differnt port
	//--ssl - use ssl or not..
	//--user - choose a user to start as
	if ( argc < 2 ) {
		fprintf( stderr, "No options received.\n" );
		const char *fmt = "  --%-10s       %-30s\n";
		fprintf( stderr, fmt, "start", "start new servers" );
		fprintf( stderr, fmt, "kill", "test killing a server" );
		fprintf( stderr, fmt, "fork", "daemonize the server" );
		fprintf( stderr, fmt, "port <arg>", "set a differnt port" );
		fprintf( stderr, fmt, "ssl", "use ssl or not.." );
		fprintf( stderr, fmt, "user <arg>", "choose a user to start as" );
		return 0;	
	}	
	else {
		while ( *argv ) {
			if ( strcmp( *argv, "--start" ) == 0 ) 
				values.start = 1;
			else if ( strcmp( *argv, "--kill" ) == 0 ) 
				values.kill = 1;
			else if ( strcmp( *argv, "--port" ) == 0 ) 
				values.port = atoi( *argv );
			else if ( strcmp( *argv, "--ssl" ) == 0 ) 
				values.ssl = 1;
			else if ( strcmp( *argv, "--daemonize" ) == 0 ) 
				values.fork = 1;
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
	}	

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		fprintf( stderr, "Invoked with options:\n" );
		fprintf( stderr, "%10s: %d\n", "start", values.start );	
		fprintf( stderr, "%10s: %d\n", "kill", values.kill );	
		fprintf( stderr, "%10s: %d\n", "port", values.port );	
		fprintf( stderr, "%10s: %d\n", "fork", values.fork );	
		fprintf( stderr, "%10s: %s\n", "user", values.user );	
		fprintf( stderr, "%10s: %s\n", "ssl", values.ssl ? "true" : "false" );	
	}


	//Set all of the socket stuff
	const int defport = 2000;
	struct sockAbstr su;
	su.addrsize = sizeof(struct sockaddr);
	su.buffersize = 1024;
	su.opened = 0;
	su.backlog = 500;
	su.waittime = 5000;
	su.protocol = IPPROTO_TCP;
	su.socktype = SOCK_STREAM;
	//su.protocol = IPPROTO_UDP;
	//su.sockettype = SOCK_DGRAM;
	su.iptype = PF_INET;
	su.reuse = SO_REUSEADDR;
	su.port = !values.port ? (int *)&defport : &values.port;
	su.ssl_ctx = NULL;

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		FILE *e = stderr;
		fprintf( e, "Socket opened with options:\n" );
		fprintf( e, "%10s: %d\n", "addrsize", su.addrsize );	
		fprintf( e, "%10s: %d\n", "buffersize", su.buffersize );	
		fprintf( e, "%10s: %d connections\n", "backlog", su.backlog );	
		fprintf( e, "%10s: %d microseconds\n", "waittime", su.waittime );	
		fprintf( e, "%10s: %s\n", "protocol", su.protocol == IPPROTO_TCP ? "tcp" : "udp" );	
		fprintf( e, "%10s: %s\n", "socktype", su.socktype == SOCK_STREAM ? "stream" : "datagram" );	 
		fprintf( e, "%10s: %s\n", "IPv6?", su.iptype == PF_INET ? "no" : "yes" );	
		fprintf( e, "%10s: %d\n", "port", *su.port );	
		//How to dump all the socket info?
	}

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
	if (setsockopt(su.fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1) {
		// su.free(sock);
		su.err = errno;
		return (0, "Could not reopen socket.");
	}
	#endif

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
		}
		else if ( cpid ) {
			//All the processing occurs here.
			struct HTTPBody rq, rs;	
			//struct aptr *aptrs[] = { ; }

			//TODO: Somewhere in here, a signal needs to run that allows this thing to die.
			//...

			//Read the message	
			if ((status = l_read( fd, &rq, &rs )) == -1 ) {
				//TODO: Besides handling errors, a lot of what comes out here will define
				//what to do with the response...
			}

			//Write a new message	
			if (( l_write( fd, &rq, &rs )) == -1 ) {
				//...
			}
				
			if ( close( fd ) == -1 ) {
				fprintf( stderr, "Couldn't close child socket. %s\n", strerror(errno) );
				return 0;
			}	
		}

	}

	//Close the server process.
	if ( close( su.fd ) == -1 ) {
		fprintf( stderr, "Couldn't close socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	return 0;
}
#endif

#include "ctx-http.h"

//No-op
void create_notls ( void **p ) { ; }

//Accepts new connections
int accept_notls ( struct sockAbstr *su, int *child, void *p, char *err, int errlen ) {
	su->addrlen = sizeof (struct sockaddr);	
	//Accept a connection if possible...
	if (( *child = accept( su->fd, &su->addrinfo, &su->addrlen )) == -1 ) {
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


//Read a message that the server will use later.
int read_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );
#if 1
	//Read all the data from a socket.
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	
	char err[ 2048 ] = {0};

	//Read first
	for ( ;; ) {	
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//Read into a static buffer
		if ( ( rd = recv( fd, buf2, size, MSG_DONTWAIT ) ) == -1 ) {
			//A subsequent call will tell us a lot...
			FPRINTF( "Couldn't read all of message...\n" );
			//whatsockerr( errno );
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF("Tried three times to read from socket. We're done.\n" );
					FPRINTF("rq->mlen: %d\n", rq->mlen );
					FPRINTF("%p\n", buf );
					//rq->msg = buf;
					break;
				}
				FPRINTF("Tried %d times to read from socket. Trying again?.\n", try );
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
		else {
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
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
#endif
	FPRINTF( "Read complete.\n" );
	return 1;
}


//Write
int write_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Write started...\n" );
	int sent = 0;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

	for ( ;; ) {	
		sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT );
		FPRINTF( "Bytes sent: %d\n", sent );
		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			return 1;
		}
		else if ( sent > -1 ) {
			//continue resending...
			pos += sent;
			total -= sent;	
		}
		else /*if ( sent == -1 )*/ {
			//TODO: Can't close a most-likely closed socket.  What do you do?
			if ( errno == EBADF )
				return 0;
			else if ( errno == ECONNREFUSED )
				return 0;
			else if ( errno == EFAULT )
				return 0;
			else if ( errno == EINTR )
				return 0;
			else if ( errno == EINVAL )
				return 0;
			else if ( errno == ENOMEM )
				return 0;
			else if ( errno == ENOTCONN )
				return 0;
			else if ( errno == ENOTSOCK )
				return 0;
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF( "Tried three times to write to socket. We're done.\n" );
					FPRINTF( "rs->mlen: %d\n", rs->mlen );
					//rq->msg = buf;
					return 0;	
				}
				FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
			}
			else {
				//this would just be some uncaught condition...
				FPRINTF( "Caught some unknown condition.\n" );
			}
		}
	}
	FPRINTF( "Write complete.\n" );
	return 1;
}



//Destroy anything
void free_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Deallocation started...\n" );

	//Free the HTTP body 
	http_free_body( rs );
	http_free_body( rq );

	//Close the file
	if ( close( fd ) == -1 ) {
		FPRINTF( "Couldn't close child socket. %s\n", strerror(errno) );
	}

	FPRINTF( "Deallocation complete.\n" );
}



#include "ctx-http.h"

//No-op
void create_notls ( void **p ) { ; }


//Read a message that the server will use later.
int read_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );

	//Read all the data from a socket.
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	
	char err[ 2048 ] = {0};

	//Read first
	for ( ;; ) {

		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );
		int bfsize = size * ( ++mult ); 
		int rd = recv( fd, buf2, size, MSG_DONTWAIT );

		//Read into a static buffer
		if ( rd == -1 && try )
			break;
		else if ( rd == 0 ) {
			rq->msg = buf;
			break;
		}
		else if ( rd == -1 && !try ) {
			//A subsequent call will tell us a lot...
			FPRINTF( "Couldn't read all of message...\n" );
			//whatsockerr( errno );
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF( "Tried three times to read from socket. We're done.\n" );
					break;
				}
				FPRINTF("Tried %d times to read from socket. Got %d bytes.\n", try, rd );
			}
			else {
				//this would just be some uncaught condition...
				return http_set_error( rs, 500, strerror( errno ) );
			}
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
			try++;

			//show read progress and data received, etc.
			FPRINTF( "Received %d bytes on fd %d\n", rd, fd ); 
		}
	}

	if ( !http_parse_request( rq, err, sizeof(err) ) ) {
		return http_set_error( rs, 500, err ); 
	}

	FPRINTF( "Read complete.\n" );
	return 1;
}


//Write
int write_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Write started...\n" );
	int sent = 0, pos = 0, try = 0;
	int total = rs->mlen;

	for ( ;; ) {	
		sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT );
		FPRINTF( "Bytes sent: %d\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			return 1;
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
		}
		else if ( sent == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
			FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
		}
		else {
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
			else {
				//this would just be some uncaught condition...
				FPRINTF( "Caught some unknown condition.\n" );
			}
		}

		try++;
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}
	FPRINTF( "Write complete.\n" );
	return 1;
}


#if 0
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
#endif


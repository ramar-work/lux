#include "ctx-http.h"

//No-op
void create_notls ( void **p ) { ; }


//Read a message that the server will use later.

//TODO
//Why not read directly into rq->msg?
//Still not sure how to read all of the data from an incoming request, but we can't stop until it's done...
//Can we reallocate instead of reading to buffer and copying?

int read_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );
	const int size = 1024; 
	int mult = 1;
	char err[ 2048 ] = {0};

	//Read first
	for ( ;; ) {

		int rd, nsize = size * mult;
		unsigned char *ptr = NULL;
		FPRINTF( "const bsize = %d, msgbuf size = %d, start pos = %d\n", size, nsize, nsize - size );

		if ( ( ptr = rq->msg = realloc( rq->msg, nsize ) ) == NULL ) { 
			return http_set_error( rs, 500, "Could not allocate read buffer." ); 
		}

		if ( !memset( ptr += (nsize - size), 0, size ) ) {
			return http_set_error( rs, 500, "Could not zero out new read buffer." ); 
		}

	#if 1
		//Read a message
		rd = recv( fd, ptr, size, MSG_DONTWAIT );
	
		#if 0
		FPRINTF( "recv returned '%d'...\n", rd );
		write( 2, ptr, rd );
		FPRINTF( "supplied buffer (rq->msg) looks like: %p\n", rq->msg );	
		getchar();
		#endif

		//Read into a static buffer
		if ( rd == 0 ) {
	#else
		if ( ( rd = recv( fd, ptr, size, MSG_DONTWAIT ) ) == 0 ) {
	#endif
			break;
		}
		else if ( rd == -1 ) {
			//A subsequent call will tell us a lot...
			FPRINTF( "Couldn't read all of message...\n" );
			//whatsockerr( errno );
			if ( errno == EAGAIN || errno == EWOULDBLOCK )
				FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			else {
				//All of this errors result in a 500
				//( errno == EBADF || ECONNREFUSED || EFAULT || EINTR || EINVAL || ENOMEM || ENOTCONN || ENOTSOCK )
				return http_set_error( rs, 500, strerror( errno ) );
			}
		}
		else {
			//Define a temporary body
			struct HTTPBody *tmp;

			//Increment message length
			rq->mlen += rd;

			//We have to try to parse, the first go MAY not work
			tmp = http_parse_request( rq, err, sizeof(err) ); 
			print_httpbody( tmp );

			if ( tmp->error == ZHTTP_NONE ) { 
				FPRINTF( "All data received\n" );
				break;
			}
			else if ( tmp->error == ZHTTP_INCOMPLETE_HEADER ) {
				FPRINTF( "Got non-fatal HTTP parser error: %s.  Try reading again...\n", err );
			}
			else {
				//send a 500 with what went wrong
				FPRINTF( "Got fatal HTTP parser error: %s\n", err );
				return http_set_error( rs, 500, err ); 
			}
			mult++;
			//show read progress and data received, etc.
			FPRINTF( "Received %d bytes on fd %d\n", rd, fd ); 
getchar();
		}
	}

	FPRINTF( "Read complete.\n" );
close(fd);
exit(0);
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


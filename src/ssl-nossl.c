#include "ssl-nossl.h"
/*ssl-nossl.c*/

void * create_nossl() { return NULL; };
void destroy_nossl( void * ) { return NULL; };

int accept_nossl ( struct sockAbstr *su, int *child, char *err, int errlen ) {
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


int read_nossl( int fd, void *ctx, uint8_t *buffer, int len ) {
	int rd;
	//Read into a static buffer
	if ( rd == -1 ) {
		//A subsequent call will tell us a lot...
		FPRINTF( "Couldn't read all of message...\n" );
#if 0
		if ( errno == EBADF )
			0; //TODO: Can't close a most-likely closed socket.  What do you do?
		else if ( errno == ECONNREFUSED ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == EFAULT ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == EINTR ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == EINVAL ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == ENOMEM ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == ENOTCONN ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else if ( errno == ENOTSOCK ) 
			return http_set_error( rs, 500, strerror( errno ) );
		else 
#endif
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			return RD_EAGAIN;
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
	return 1;
}


int write_nossl( int fd, void *ctx, uint8_t *buffer, int len ) {
//int write_nossl( int fd, uint8_t *buf, int *buflen, int *sent ) {
	*sent = send( fd, buf, *buflen, MSG_DONTWAIT );
	if ( *sent == -1 ) {
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			return WR_EAGAIN;
		}
		else if ( errno == EBADF ) {
			//TODO: When would a socket close early like this?
			FPRINTF( "Connection refused." );
			return 0;
		}
		else if ( errno == ECONNREFUSED ) {
			FPRINTF( "Connection refused." );
			return 0;
		}
		else if ( errno == EFAULT ) {
			FPRINTF( "EFAULT." );
			return 0;
		}
		else if ( errno == EINTR ) {
			FPRINTF( "EINTR." );
			return 0;
		}
		else if ( errno == EINVAL ) {
			FPRINTF( "EINVAL." );
			return 0;
		}
		else if ( errno == ENOMEM ) {
			FPRINTF( "Out of memory." );
			return 0;
		}
		else if ( errno == ENOTCONN ) {
			return 0;
		}
		else if ( errno == ENOTSOCK ) {
			return 0;
		}
		else {
			//this would just be some uncaught condition...
			return 0;
		}
	}
	return 1;
}



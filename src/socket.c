/* -------------------------------------------------------- *
 * socket.c
 * ========
 * 
 * Summary 
 * -------
 * Socket abstractions.
 * NOTE: May retire these in the future. 
 * 
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 * 
 * Changelog 
 * ----------
 * 
 * -------------------------------------------------------- */
#include "socket.h"

#if 0
//Return the total bytes read.
int read_from_socket ( int fd, uint8_t **b, void (*readMore)(int *, void *) ) {
	return 0;
	int mult = 0;
	int try=0;
	int mlen = 0;
	const int size = 32767;	
	uint8_t *buf = malloc( 1 );

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//read into new buffer
		//if (( rd = read( fd, &buf[ rq->mlen ], size )) == -1 ) {
		//TODO: Yay!  This works great on Arch!  But let's see what about Win, OSX and BSD
		if (( rd = recv( fd, buf2, size, MSG_DONTWAIT )) == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message...\n" );
			//whatsockerr(errno);
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
					fprintf(stderr, "mlen: %d\n", mlen );
					//fprintf(stderr, "%p\n", buf );
					//rq->msg = buf;
					break;
			}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
				fprintf(stderr, "Uncaught condition at read!\n" );
				return 0;
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			//rq->msg = buf;
			break;
		}
		else {
			//realloc manually and read
			if (( b = realloc( b, bfsize )) == NULL ) {
				fprintf(stderr, "Couldn't allocate buffer..." );
				close(fd);
				return 0;
			}

			//Copy new data and increment bytes read
			memset( &b[ bfsize - size ], 0, size ); 
			fprintf(stderr, "pos: %d\n", bfsize - size );
			memcpy( &b[ bfsize - size ], buf2, rd ); 
			mlen += rd;
			//rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			if ( 1 ) {
				fprintf( stderr, "Recvd %d bytes on fd %d\n", rd, fd ); 
			}
		}
	}

	return mlen;
}


int write_to_socket ( int fd, uint8_t *b ) {
	return 0;
}

int sread_from_socket ( int fd, uint8_t *b ) {
	return 0;
}

int swrite_to_socket ( int fd, uint8_t *b ) {
	return 0;
}
#endif


//tell me the error returned.  I don't know what happened...
void whatsockerr( int e ) {
	if ( e == EBADF  ) fprintf( stderr, "Got sockerr: %s", "EBADF " );
	else if ( e == ECONNREFUSED  ) fprintf( stderr, "Got sockerr: %s", "ECONNREFUSED " );
	else if ( e == EFAULT  ) fprintf( stderr, "Got sockerr: %s", "EFAULT " );
	else if ( e == EINTR  ) fprintf( stderr, "Got sockerr: %s", "EINTR " );
	else if ( e == EINVAL  ) fprintf( stderr, "Got sockerr: %s", "EINVAL " );
	else if ( e == ENOMEM  ) fprintf( stderr, "Got sockerr: %s", "ENOMEM " );
	else if ( e == ENOTCONN  ) fprintf( stderr, "Got sockerr: %s", "ENOTCONN " );
	else if ( e == ENOTSOCK  ) fprintf( stderr, "Got sockerr: %s", "ENOTSOCK " );
	else if ( e == EAGAIN || e == EWOULDBLOCK  ) fprintf( stderr, "Got sockerr: %s", "EAGAIN || EWOULDBLOCK " );
}


void print_socket ( struct sockAbstr *sa ) {
	const char *fmt = "%-10s: %s\n";
	FILE *e = stderr;
	fprintf( e, "Socket opened with options:\n" );
	fprintf( e, "%10s: %d\n", "addrsize", sa->addrsize );	
	fprintf( e, "%10s: %d\n", "buffersize", sa->buffersize );	
	fprintf( e, "%10s: %d connections\n", "backlog", sa->backlog );	
	fprintf( e, "%10s: %d microseconds\n", "waittime", sa->waittime );	
	fprintf( e, "%10s: %s\n", "protocol", ( sa->protocol == IPPROTO_TCP ) ? "tcp" : "udp" );	
	fprintf( e, "%10s: %s\n", "socktype", ( sa->socktype == SOCK_STREAM ) ? "stream" : "datagram" );	 
	fprintf( e, "%10s: %s\n", "IPv6?", ( sa->iptype == PF_INET ) ? "no" : "yes" );	
	fprintf( e, "%10s: %d\n", "port", *sa->port );	
}


struct sockAbstr * populate_socket ( struct sockAbstr *sa, int protocol, int socktype, int *port ) {
	sa->addrsize = sizeof(struct sockaddr);
	sa->buffersize = 1024;
	sa->opened = 0;
	sa->backlog = 500;
	sa->waittime = 5000;
	sa->protocol = protocol;
	sa->socktype = socktype;
	sa->iptype = PF_INET;
	sa->reuse = SO_REUSEADDR;
	sa->port = port;
	memset( sa->iip, 0, 16 ); 
	return sa;
}


int get_iip_of_socket( struct sockAbstr *sa ) {
	//struct sockaddr_in *cin = (struct sockaddr_in *)&a->addrinfo;
	char *ip = inet_ntoa( sa->sin->sin_addr );
	memcpy( sa->iip, ip, strlen( ip ) );
	return 1;
}


struct sockAbstr * set_timeout_on_socket ( struct sockAbstr *sa, int timeout ) {
	#if 0
	//Set timeout, reusable bit and any other options 
	struct timespec to = { .tv_sec = 2 };
	if ( setsockopt(sa->fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1 ) {
		// sa->free(sock);
		sa->err = errno;
		return (0, "Could not reopen socket.");
	}
	#endif
	return NULL;
}
  

struct sockAbstr * set_nonblock_on_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	if ( fcntl( sa->fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		snprintf( err, errlen, "fcntl error: %s\n", strerror(errno) ); 
		return NULL;
	}
	return sa;
}


struct sockAbstr * open_listening_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	int status;
	struct sockaddr_in *si = &sa->sin2; 

#if 0
	if ( !( si = malloc( sizeof( struct sockaddr_in ) ) ) ) {
		snprintf( err, errlen, "Couldn't allocate space for socket: %s\n", strerror( errno ) );
		return NULL;	
	}
#endif

	si->sin_family = sa->iptype; 
	si->sin_port = htons( *sa->port );
	(&si->sin_addr)->s_addr = htonl( INADDR_ANY ); // Can't this fail?

	if (( sa->fd = socket( sa->iptype, sa->socktype, sa->protocol )) == -1 ) {
		snprintf( err, errlen, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		return NULL;
	}

#if 1
	if ( fcntl( sa->fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		snprintf( err, errlen, "fcntl error: %s\n", strerror(errno) ); 
		return NULL;
	}
#endif

	if (( status = bind( sa->fd, (struct sockaddr *)si, sizeof(struct sockaddr_in))) == -1 ) {
		snprintf( err, errlen, "Couldn't bind socket to address! Error: %s\n", strerror( errno ) );
		return NULL;
	}

	if (( status = listen( sa->fd, sa->backlog) ) == -1 ) {
		snprintf( err, errlen, "Couldn't listen for connections! Error: %s\n", strerror( errno ) );
		return NULL;
	}

	sa->sin = si;
	sa->opened = 1;
	return sa;
}


struct sockAbstr * close_listening_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	if ( close( sa->fd ) == -1 ) {
		snprintf( err, errlen, "Could not close socket file %d: %s\n", sa->fd, strerror(errno) ); 
		sa->sin = NULL;
		return NULL;
	}

	sa->sin = NULL;
	return sa;
}


struct sockAbstr * accept_listening_socket ( struct sockAbstr *sa, int *fd, char *err, int errlen ) {
	if (( *fd = accept( sa->fd, &sa->addrinfo, &sa->addrlen ) ) == -1 ) {
		//TODO: Need to check if the socket was non-blocking or not...
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			//This should just try to read again
			snprintf( err, errlen, "Try accept again: %s\n", strerror( errno ) );
			return NULL;	
		}
		else if ( errno == EMFILE || errno == ENFILE ) { 
			//These both refer to open file limits
			snprintf( err, errlen, "Too many open files, try closing some requests.\n" );
			return NULL;
		}
		else if ( errno == EINTR ) { 
			//In this situation we'll handle signals
			snprintf( err, errlen, "Signal received: %s\n", strerror( errno ) );
			return NULL;
		}
		else {
			//All other codes really should just stop. 
			snprintf( err, errlen, "accept() failed: %s\n", strerror( errno ) );
			return NULL;
		}
	}
	return sa;
}


struct sockAbstr * open_connecting_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	return NULL;
}


struct sockAbstr * close_connecting_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	return NULL;
}

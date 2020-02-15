#include "socket.h"

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


struct sockAbstr * populate_socket ( struct sockAbstr *sa ) {
	sa->addrsize = sizeof(struct sockaddr);
	sa->buffersize = 1024;
	sa->opened = 0;
	sa->backlog = 500;
	sa->waittime = 5000;
	sa->protocol = IPPROTO_TCP;
	sa->socktype = SOCK_STREAM;
	//sa->protocol = IPPROTO_UDP;
	//sa->sockettype = SOCK_DGRAM;
	sa->iptype = PF_INET;
	sa->reuse = SO_REUSEADDR;
	//sa->port = !values.port ? (int *)&defport : &values.port;
	sa->ssl_ctx = NULL;
	return sa;
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
	struct sockaddr_in t;
	struct sockaddr_in *si = NULL; 

	//An alternate way to do this is memcpy of a static structure to alloc	
	//si->sin_family = sa->iptype; 
	//si->sin_port = htons( *sa->port );
	//(&si->sin_addr)->s_addr = htonl( INADDR_ANY ); // Can't this fail?

	if ( !( si = malloc( sizeof( struct sockaddr_in ) ) ) ) {
		snprintf( err, errlen, "Couldn't allocate space for socket: %s\n", strerror( errno ) );
		return NULL;	
	}

	si->sin_family = sa->iptype; 
	si->sin_port = htons( *sa->port );
	(&si->sin_addr)->s_addr = htonl( INADDR_ANY ); // Can't this fail?

	if (( sa->fd = socket( sa->iptype, sa->socktype, sa->protocol )) == -1 ) {
		snprintf( err, errlen, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		return NULL;
	}

	if ( fcntl( sa->fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		snprintf( err, errlen, "fcntl error: %s\n", strerror(errno) ); 
		return NULL;
	}

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
		return sa;
	}

	free( sa->sin );
	sa->sin = NULL;
	return NULL;
}


struct sockAbstr * open_connecting_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	return NULL;
}


struct sockAbstr * close_connecting_socket ( struct sockAbstr *sa, char *err, int errlen ) {
	return NULL;
}

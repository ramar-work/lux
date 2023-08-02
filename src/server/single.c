#include "single.h"

int srv_single ( server_t *p ) {
#if 1
#if 0
	for ( int fd, count = 0, i = 0; i < LEAKLIMIT; i++ ) { 
#else
	for ( int count = 0; ; ) {
#endif
		//Client address and length?
		char ip[ 128 ] = { 0 };
		struct sockaddr_storage addrinfo = { 0 };
		socklen_t addrlen = sizeof( addrinfo );


		// Create a structure for connection info 
		//struct cdata conn = { .ctx = p->ctx };
		conn_t conn;
		memset( &conn, 0, sizeof( conn_t ) );
		conn.count = 0;
		conn.data = NULL;
		//conn.start = 0;
		//conn.end = 0;


		//Accept a new connection	
		if ( ( conn.fd = accept( p->fd, (struct sockaddr *)&addrinfo, &addrlen ) ) == -1 ) {
			//TODO: Need to check if the socket was non-blocking or not...
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				//This should just try to read again
				snprintf( p->err, sizeof( p->err ), "Try accept again: %s\n", strerror( errno ) );
				continue;	
			}
			else if ( errno == EMFILE || errno == ENFILE ) { 
				//These both refer to open file limits
				snprintf( p->err, sizeof( p->err ), "Too many open files, try closing some requests.\n" );
				//fprintf( stderr, "%s\n", err );
				fprintf( p->log_fd, "%s\n", p->err );
				return 0;
			}
			else if ( errno == EINTR ) { 
				//In this situation we'll handle signals
				snprintf( p->err, sizeof( p->err ), "Signal received: %s\n", strerror( errno ) );
				fprintf( p->log_fd, "%s", p->err );
				return 0;
			}
			else {
				//All other codes really should just stop. 
				snprintf( p->err, sizeof( p->err ), "accept() failed: %s\n", strerror( errno ) );
				fprintf( p->log_fd, "%s", p->err );
				return 0;
			}
		}

		//Log an access message including the IP in either ipv6 or v4
		if ( addrinfo.ss_family == AF_INET )
			inet_ntop( AF_INET, &((struct sockaddr_in *)&addrinfo)->sin_addr, ip, sizeof( ip ) ); 
		else {
			inet_ntop( AF_INET6, &((struct sockaddr_in6 *)&addrinfo)->sin6_addr, ip, sizeof( ip ) ); 
		}

		// Go ahead and service it
		if ( !srv_response( p, &conn ) ) {
			FPRINTF( "I'm so lost... \n" );
		}

		// Close the file descriptor
		if ( close( conn.fd ) == -1 ) {
			//TODO: You need to handle this and retry closing to regain resources
			FPRINTF( "Error when closing child socket.\n" );
		}

		// Keep track of connections
		count++;
		FPRINTF( "Waiting for next connection.\n" );
		#ifdef DEBUG_H
		if ( count >= p->tapout ) { 
			break;
		}	
		#endif
	}
#endif
	return 1;
}

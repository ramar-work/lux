#include "multithread.h"

// TODO: Use conn_t instead of this
struct threadinfo_t _fds[ MAX_THREADS ] = { { -1, 0, -1, { 0 } } };

// Wait one second between
static const struct timespec __interval__ = { 1, 0 };

// A server as a function for pthread_create 
static void * server_proc( void *t ) {
	//struct cdata conn = { .ctx = ctx, .flags = O_NONBLOCK };
	//struct cdata conn = { .ctx = f->ctx, .flags = O_NONBLOCK };
	struct threadinfo_t *tt = (struct threadinfo_t *)t; 
	tt->running = THREAD_ACTIVE;
	//struct threadinfo_t *f = (struct threadinfo_t *)t;	
	//struct cdata conn = { .ctx = tt->ctx, .flags = O_NONBLOCK };
	server_t *p = tt->server;

	// Set up ocnnection data;
	conn_t conn = {0};
	conn.count = 0;
	conn.status = 0;
	conn.data = NULL;
	//conn.start = 0;
	//conn.end = 0;
	conn.fd = tt->fd;

	// TODO: Remove this, and use the conn structure directly
	memcpy( conn.ipv4, tt->ipaddr, strlen( tt->ipaddr ) );

	// Send a response
	if ( !srv_response( p, &conn ) ) {
		//snprintf( conn.err, sizeof( conn.err ), "Error in TCP socket handling.\n" );
		FPRINTF( "%s\n", conn.err );
		//pthread_exit(0);
		pthread_exit(0);
	}

	// Close a file
	if ( close( conn.fd ) == -1 ) {
		snprintf( conn.err, sizeof( conn.err ), 
			"Error closing TCP socket connection: %s\n", strerror( errno ) );
		FPRINTF( "%s\n", conn.err );
	}

	#if 0
	// Manage long running connections
	if ( close( fd ) == -1 ) {
		FPRINTF( "Error when closing child socket.\n" );
		//TODO: You need to handle this and retry closing to regain resources
	}
	for ( ;; ) {
		if ( !srv_response( fd, &conn ) ) {
			FPRINTF( "Error in TCP socket handling.\n" );
		}

		if ( CONN_CLOSE || conn.count < 0 || conn.count > 5 ) {
			FPRINTF( "Closing connection marked by descriptor %d to peer.\n", fd );
			if ( close( fd ) == -1 ) {
				FPRINTF( "Error when closing child socket.\n" );
				//TODO: You need to handle this and retry closing to regain resources
			}
			FPRINTF( "Connection is done. count is %d\n", conn.count );
			FPRINTF( "returning NULL.\n" );
			tt->running = THREAD_INACTIVE;
			return 0;
		}
	}
	#endif
	FPRINTF( "Child process is exiting.\n" );
	tt->running = THREAD_INACTIVE;

	// For right now, this will prevent high memory usage...
	return 0;
}


// EINTR will handle hangups
#if 0
	// Make a note of what the current count is
	for ( int top = count, i = top; i > -1; i-- ) {
		if ( _fds[ i ].running == THREAD_AVAILABLE )
			connindex = i;
		else if ( _fds[ i ].running == THREAD_INACTIVE ) {
			// Minus one client
			count--;
			connindex = i;
			_fds[ i ].running = THREAD_AVAILABLE;
			pthread_join( _fds[ i ].id, NULL ); 
		}
		#if 0
		else {
		// This is where we may need force closure
		int s = pthread_join( _fds[ count ].id, NULL ); 
		}
		#endif
	}
#endif



// A multithreaded server
//int srv_multithread( struct something_t *b, char *err, int errlen ) {
int srv_multithread( server_t *p ) {

	// Define
	pthread_attr_t attr;
	const short int client_max = p->max_per; // CLIENT_MAX_SIMULTANEOUS

	//Initialize thread attribute thing
	if ( pthread_attr_init( &attr ) != 0 ) {
		FPRINTF( "Failed to initialize thread attributes: %s\n", strerror(errno) );
		return 0;	
	}

	//Set a minimum stack size (1kb?)
	if ( pthread_attr_setstacksize( &attr, STACK_SIZE ) != 0 ) {
		FPRINTF( "Failed to set new stack size: %s\n", strerror(errno) );
		return 0;	
	}

	// Wait for connections
	for ( int fd = 0, count = 0, connindex = 0; ; ) {
		// Client address and length?
		char ip[ 128 ] = { 0 };
		struct sockaddr_storage addrinfo = { 0 };
		struct threadinfo_t *f = NULL; 
		socklen_t addrlen = sizeof( addrinfo );

		//FPRINTF( "CONNECTION COUNT SO FAR: %d out of %d\n", count, MAX_THREADS );
		if ( count >= client_max ) {
			int top = count;
			do {
				for ( int i = top; i > -1; i-- ) {
					if ( _fds[ i ].running == THREAD_AVAILABLE )
						connindex = i;
					else if ( _fds[ i ].running == THREAD_INACTIVE ) {
						count--;
						connindex = i;
						_fds[ i ].running = THREAD_AVAILABLE;
						pthread_join( _fds[ i ].id, NULL ); 
					}
					#if 0
					else {
					// This is where we may need force closure
					int s = pthread_join( _fds[ count ].id, NULL ); 
					}
					#endif
				}

				// We'll need to run the loop again
				// nanosleep( &__interval__, NULL );
			} while ( p->interrupt && count ); /// && !count );

		#if 0
			// TODO: Test for tapout or interrupt
			if ( !count ) {
				FPRINTF( "sick of waiting...\n" );
				return 1;
			}
		#endif
		}


		// Find an availalbe slot, and move forward when we're ready
		for ( ; connindex < client_max; connindex++ ) {	
			f = &_fds[ connindex ];
			FPRINTF( "Searching for available slot: %d\n", connindex );
			if ( f->running == THREAD_AVAILABLE ) {
				f->server = p;
				FPRINTF( "Using available slot %d!\n", connindex );
				break;
			}
		}

		// Accept a new connection	
		if ( f && ( f->fd = accept( p->fd, (struct sockaddr *)&addrinfo, &addrlen ) ) == -1 ) {
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

		// Only go here if we successfully accepted
		if ( f && f->fd ) {
			//Log an access message including the IP in either ipv6 or v4
			if ( addrinfo.ss_family == AF_INET )
				inet_ntop( AF_INET, &((struct sockaddr_in *)&addrinfo)->sin_addr, ip, sizeof( ip ) ); 
			else {
				inet_ntop( AF_INET6, &((struct sockaddr_in6 *)&addrinfo)->sin6_addr, ip, sizeof( ip ) ); 
			}

			//Populate any other thread data
			memcpy( f->ipaddr, ip, sizeof( ip ) );
			//FPRINTF( "IP addr is: %s\n", ip );

			//Increment both the index and the count here
			connindex++, count++;

			//Start a new thread
			FPRINTF( "Starting new thread...\n" );
			if ( pthread_create( &f->id, NULL, server_proc, f ) != 0 ) {
				snprintf( p->err, sizeof( p->err ), 
					"pthread_create unsuccessful: %s\n", strerror( errno ) );
				continue;
			}
		}
		FPRINTF( "Waiting for next connection...\n" );
	}

	return 1;
}

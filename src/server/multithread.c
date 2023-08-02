#include "multithread.h"

// Initialize a set of connections
static conn_t _fds[ 256 ] = { 0 };

// Wait one second between
static const struct timespec __interval__ = { 1, 0 };

// Initialize a new connection 
static void init_conn_after_accept( server_t *p, conn_t *conn ) {
	conn->server = p;
	conn->count = 0;
	conn->server = p;
	conn->running = CONNSTAT_ACTIVE;
	conn->data = NULL;
	conn->stage = CONN_DORMANT;
	conn->retry = 0;
}


static void start_timer_per_thread( conn_t *conn ) {
	memset( &conn->start, 0, sizeof( struct timespec ) );
	clock_gettime( CLOCK_REALTIME, &conn->start );
}


static void write_to_access_log ( server_t *p, conn_t *conn ) {
	char timebuf[ 64 ] = {0};
	time_format( &conn->start, timebuf, sizeof( timebuf ) );
	fprintf( p->access_fd, "%s\t%s\n", conn->ipv4, timebuf );
}


#if 0
// Runs in the background and adjusts the available pool
static void * reaper() {
	if ( count >= client_max ) {
		int top = count;
		do {
			for ( int i = top; i > -1; i-- ) {
				if ( _fds[ i ].running == CONNSTAT_AVAILABLE )
					connindex = i;
				else if ( _fds[ i ].running == CONNSTAT_INACTIVE ) {
					count--;
					connindex = i;
					_fds[ i ].running = CONNSTAT_AVAILABLE;
					pthread_join( _fds[ i ].id, NULL ); 
				}
				else {
					//This is your last line of defense.
					//If nothing happens for a specified timeout, kill it

					struct timespec end = {0};
					clock_gettime( CLOCK_REALTIME, &end );

					FPRINTF( "thread at slot %d, up for %d seconds\n", 
							i, time_diff_sec( &_fds[i].start, &end  ) );

					// this is assuming a lot, so check
					// current status vs past status
					if ( time_diff_sec( &_fds[i].start, &end ) > 15 ) {
						// Try just closing the fd (it's not in use though...)
						if ( close( _fds[ i ].fd ) == -1 )
							FPRINTF( "Failed to close open file %d: %s\n", _fds[ i ].fd, strerror( errno ) );
						// Or just ctx->cleanup
						
						// What exactly happened?
						if ( pthread_cancel( _fds[ i ].id ) != 0 ) {
							FPRINTF( "Attempted to kill thread %d: %s\n", i, strerror( errno ) );
						}

						// All should probably be memset to zero
						_fds[ i ].running = CONNSTAT_AVAILABLE;
					}
				}
			}

			// We'll need to run the loop again
			nanosleep( &__interval__, NULL );
		} while ( p->interrupt && count ); /// && !count );
	#if 0
		// TODO: Test for tapout or interrupt
		if ( !count ) {
			FPRINTF( "sick of waiting...\n" );
			return 1;
		}
	#endif
	}
	return NULL;
}	
#endif



// A server as a function for pthread_create 
static void * server_proc( void *t ) {

	// Define
	conn_t *conn = (conn_t *)t;

	// Send a response
	if ( !srv_response( conn->server, conn ) ) {
		//We let the reaper do it's thing...
		//snprintf( conn.err, sizeof( conn.err ), "Error in TCP socket handling.\n" );
	}

	// Close a file
	if ( close( conn->fd ) == -1 ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"Error closing TCP socket connection: %s\n", strerror( errno ) );
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

		if ( CONNSTAT_CLOSE || conn.count < 0 || conn.count > 5 ) {
			FPRINTF( "Closing connection marked by descriptor %d to peer.\n", fd );
			if ( close( fd ) == -1 ) {
				FPRINTF( "Error when closing child socket.\n" );
				//TODO: You need to handle this and retry closing to regain resources
			}
			FPRINTF( "Connection is done. count is %d\n", conn.count );
			FPRINTF( "returning NULL.\n" );
			tt->running = CONNSTAT_INACTIVE;
			return 0;
		}
	}
	#endif

	conn->running = CONNSTAT_INACTIVE;
	FPRINTF( "Child process is exiting.\n" );
	return 0;
}



// A multithreaded server
int srv_multithread( server_t *p ) {

	// Define
	pthread_attr_t attr;
	const short int client_max = p->max_per; 

	// Initialize our connection structures
	memset( _fds, 0, sizeof( _fds ) );

	// Initialize thread attribute structure 
	if ( pthread_attr_init( &attr ) != 0 ) {
		FPRINTF( "Failed to initialize thread attributes: %s\n", strerror(errno) );
		return 0;	
	}

	// Set a minimum stack size (1kb?)
	if ( pthread_attr_setstacksize( &attr, STACK_SIZE ) != 0 ) {
		FPRINTF( "Failed to set new stack size: %s\n", strerror(errno) );
		return 0;	
	}

	// Wait for connections
	for ( int fd = 0, conncount = 0, connindex = 0; ; ) {
		// Client address and length?
		struct sockaddr_storage addrinfo = { 0 };
		//struct threadinfo_t *f = NULL; 
		conn_t *f = NULL;
		socklen_t addrlen = sizeof( addrinfo );

		//FPRINTF( "CONNSTATECTION COUNT SO FAR: %d out of %d\n", count, MAX_THREADS );
		//
		//Watching for this in the background is a better way to approach.
		//It's at least 2 threads now though...
	#if 0
	#else
		if ( conncount >= client_max ) {
			int top = conncount;
			do {
				for ( int i = top; i > -1; i-- ) {
					if ( _fds[ i ].running == CONNSTAT_AVAILABLE )
						connindex = i;
					else if ( _fds[ i ].running == CONNSTAT_INACTIVE ) {
						conncount--;
						connindex = i;
						_fds[ i ].running = CONNSTAT_AVAILABLE;
						pthread_join( _fds[ i ].id, NULL ); 
					}
					else {
						//This is your last line of defense.
						//If nothing happens for a specified timeout, kill it

						struct timespec end = {0};
						clock_gettime( CLOCK_REALTIME, &end );

						FPRINTF( "thread at slot %d, up for %d seconds\n", 
								i, time_diff_sec( &_fds[i].start, &end  ) );

						// this is assuming a lot, so check
						// current status vs past status
						if ( time_diff_sec( &_fds[i].start, &end ) > p->ttimeout ) {
							// Try just closing the fd (it's not in use though...)
							if ( close( _fds[ i ].fd ) == -1 ) {
								FPRINTF( "Failed to close open file %d: %s\n", _fds[ i ].fd, strerror( errno ) );
							}
							// Or just ctx->cleanup
							
							// What exactly happened?
							if ( pthread_cancel( _fds[ i ].id ) != 0 ) {
								FPRINTF( "Attempted to kill thread %d: %s\n", i, strerror( errno ) );
							}

							// All should probably be memset to zero
							_fds[ i ].running = CONNSTAT_AVAILABLE;
						}
					}
				}

				// We'll need to run the loop again
				nanosleep( &__interval__, NULL );
			} while ( p->interrupt && conncount ); /// && !conncount );
		#if 0
			// TODO: Test for tapout or interrupt
			if ( !conncount ) {
				FPRINTF( "sick of waiting...\n" );
				return 1;
			}
		#endif
		}
	#endif

		// Find an availalbe slot, and move forward when we're ready
		//FPRINTF( "(CI = %d) == (CM = %d)\n", connindex, client_max );
		for ( ; connindex < client_max; connindex++ ) {	
			f = &(_fds[ connindex ]);
			FPRINTF( "Slot %d available: %s\n", connindex, f->running == CONNSTAT_AVAILABLE ? "Y" : "N" );
			if ( f->running == CONNSTAT_AVAILABLE ) {
				f->server = p;
				FPRINTF( "Using available slot %d!\n", connindex );
				break;
			}
		}

		// Accept a new connection	
		if ( f ) {
			FPRINTF( "Waiting to accept...\n" );
			if ( ( f->fd = accept( p->fd, (struct sockaddr *)&addrinfo, &addrlen ) ) == -1 ) {
				//TODO: Need to check if the socket was non-blocking or not...
				if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
					//This should just try to read again
					snprintf( p->err, sizeof( p->err ), "Try accept again: %s\n", strerror( errno ) );
					fprintf( p->log_fd, "%s\n", p->err );
					continue;
				}
				else if ( errno == EMFILE || errno == ENFILE ) { 
					//These both refer to open file limits
					snprintf( p->err, sizeof( p->err ), "Too many open files, try closing some requests.\n" );
					//fprintf( stderr, "%s\n", err );
					fprintf( p->log_fd, "%s\n", p->err );
					continue;
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
			if ( f->fd > -1 ) {
				//We have a valid connection, so start here
				init_conn_after_accept( p, f );		
				start_timer_per_thread( f ); 

				//Log an access message including the IP in either ipv6 or v4
				if ( addrinfo.ss_family == AF_INET )
					inet_ntop( AF_INET, &((struct sockaddr_in *)&addrinfo)->sin_addr, f->ipv4, sizeof( f->ipv4 ) ); 
				else {
					//inet_ntop( AF_INET6, &((struct sockaddr_in6 *)&addrinfo)->sin6_addr, ip, sizeof( ip ) ); 
				}

				//Increment both the index and the conncount here
				conncount++;

				//Set as much connection information as you can
				f->running = CONNSTAT_ACTIVE;
				FPRINTF( "Got new connection: %d\n", f->fd );

				//Start a new thread
				if ( pthread_create( &f->id, NULL, server_proc, f ) != 0 ) {
					snprintf( p->err, sizeof( p->err ), 
						"pthread_create unsuccessful: %s\n", strerror( errno ) );
					fprintf( p->log_fd, "%s", p->err );
					f->running = CONNSTAT_AVAILABLE;
					conncount--;
				}

				//Then log as much as you can
				write_to_access_log( p, f );
			}

			FPRINTF( "ConnIndex = %d, conncount = %d\n", connindex, conncount );
			FPRINTF( "Waiting for next connection...\n" );
			// nanosleep( &__interval__, NULL );
		}
	}

	return 1;
}

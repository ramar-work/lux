/**
 * multithread.c
 * -------
 *
 * Multithreaded server logic
 *
 * Currently starts a new thread per client.  Works by
 * starting a thread and a timer for performance tracking.
 *
 * Threads that end abnormally are still tracked, but the
 * connection may be cut in a way that the client would not
 * anticipate.
 *
 */

#include "multithread.h"

// Initialize a set of connections
// TODO: Allocate this from the server start side... then you
// can effectively deal with whatever...  bring up open files
// limit and make sure there's enough space...
static conn_t _fds[ LUX_MAX_THREADS ] = { 0 };

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

// Start timer
static void start_timer_per_thread( conn_t *conn ) {
	memset( &conn->start, 0, sizeof( struct timespec ) );
	clock_gettime( CLOCK_REALTIME, &conn->start );
}

// Get a time diff of two timespec structures
static int time_diff_sec ( struct timespec *begin, struct timespec *end ) {
	return end->tv_sec - begin->tv_sec;
}

// Get a time diff of two timespec structures (returning nanoseconds)
static long time_diff_nsec ( struct timespec *begin, struct timespec *end ) {
	return end->tv_nsec - begin->tv_nsec;
}

// Format a timestamp
static int time_format ( struct timespec *timestamp, char *buf, int buflen ) {
	// pass this in and let strftime interpret?
	// or time
	if ( !timestamp || !timestamp->tv_sec ) {
		return 0;
	}

	// copy time to string
	char ibuf[ 64 ] = {0};
	snprintf( ibuf, sizeof( ibuf ), "%ld\n", timestamp->tv_sec );
	ctime_r( &timestamp->tv_sec, buf );
	return 1;
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

	// Get the end time
  // The log COULD take place here, but can SQLite sequence the writes?
	// memset( &conn->start, 0, sizeof( struct timespec ) );
	// clock_gettime( CLOCK_REALTIME, &conn->start );

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
	struct log *logger = p->logger;

	// Initialize our connection structures
	memset( _fds, 0, sizeof( _fds ) );

	// Initialize thread attribute structure
	if ( pthread_attr_init( &attr ) != 0 ) {
		FPRINTF( "Failed to initialize thread attributes: %s\n", strerror(errno) );
		return 0;	
	}

#if 0
	// Set a minimum stack size (1kb?)
	if ( pthread_attr_setstacksize( &attr, STACK_SIZE ) != 0 ) {
		FPRINTF( "Failed to set new stack size: %s\n", strerror(errno) );
		return 0;	
	}
#endif
	// Wait for connections
	for ( int fd = 0, conncount = 0, connindex = 0; ; ) {
		// Client address and length?
		struct sockaddr_storage addrinfo = { 0 };
		conn_t *f = NULL;
		socklen_t addrlen = sizeof( addrinfo );

		// Tell additional data
		FPRINTF( "conncount = %d, client max = %d\n", conncount, client_max );

		//Watching for this in the background is a better way to approach.
		//It's at least 2 threads now though...
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
						clock_gettime( CLOCK_REALTIME, &_fds[ i ].end );

						FPRINTF( "thread at slot %d, up for %d seconds\n",
								i, time_diff_sec( &_fds[i].start, &_fds[ i ].end ) );

						// this is assuming a lot, so check
						// current status vs past status
						if ( time_diff_sec( &_fds[i].start, &_fds[ i ].end ) > p->ttimeout ) {
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

		// Find an available slot, and move forward when we're ready
		//FPRINTF( "(CI = %d) == (CM = %d)\n", connindex, client_max );
		for ( ; connindex < client_max; connindex++ ) {	
			f = &( _fds[ connindex ] );
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
			f->fd = accept( p->fd, (struct sockaddr *)&addrinfo, &addrlen );

			if ( f->fd == -1 ) {
				//TODO: Need to check if the socket was non-blocking or not...
				if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
					//This should just try to read again
					snprintf( p->err, sizeof( p->err ), "Try accept again: %s\n", strerror( errno ) );
					fprintf( stderr, "%s\n", p->err );
					continue;
				}
				else if ( errno == EMFILE || errno == ENFILE ) {
					//These both refer to open file limits
					snprintf( p->err, sizeof( p->err ), "Too many open files, try closing some requests.\n" );
					//fprintf( stderr, "%s\n", err );
					fprintf( stderr, "%s\n", p->err );
					continue;
				}
				else if ( errno == EINTR ) {
					//In this situation we'll handle signals
					snprintf( p->err, sizeof( p->err ), "Signal received: %s\n", strerror( errno ) );
					fprintf( stderr, "%s", p->err );
					return 0;
				}
				else {
					//All other codes really should just stop.
					snprintf( p->err, sizeof( p->err ), "accept() failed: %s\n", strerror( errno ) );
					fprintf( stderr, "%s", p->err );
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
					fprintf( stderr, "%s", p->err );
					f->running = CONNSTAT_AVAILABLE;
					conncount--;
				}
			}

			FPRINTF( "ConnIndex = %d, conncount = %d\n", connindex, conncount );
			FPRINTF( "Waiting for next connection...\n" );
			// nanosleep( &__interval__, NULL );
		}
	}

	return 1;
}

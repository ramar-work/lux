#include "multithread.h"



struct threadinfo_t _fds[ MAX_THREADS ] = { { -1, 0, -1, { 0 } } };


//
static void timer_expired( union sigval td ) {
	struct threadinfo_t *tt = (struct threadinfo_t *)td.sival_ptr;	
#if 0
FPRINTF( "=================================\n" );
FPRINTF( "=================================\n" );
FPRINTF( "\n" );
FPRINTF( "TIMER EXPIRED....\n" );
FPRINTF( "\n" );
FPRINTF( "=================================\n" );
FPRINTF( "=================================\n" );
#endif
	tt->running = 0;
	// Force close the file?
	//close( tt->fd );	
	//tt->fd = -1; 	
}



// A server as a function for pthread_create 
static void * server_proc( void *t ) {
	//struct cdata conn = { .ctx = ctx, .flags = O_NONBLOCK };
	//struct cdata conn = { .ctx = f->ctx, .flags = O_NONBLOCK };
	struct threadinfo_t *tt = (struct threadinfo_t *)t; 
	fprintf( stderr, "THREAD DATA %p\n", tt );
	tt->running = 1;
	//struct threadinfo_t *f = (struct threadinfo_t *)t;	
	//struct cdata conn = { .ctx = tt->ctx, .flags = O_NONBLOCK };
	server_t *p = tt->server;
	fprintf( stderr, "CTX DATA %p\n", p->ctx );
	conn_t conn;
	memset( &conn, 0, sizeof( conn_t ) );
	conn.count = 0;
	conn.status = 0;
	conn.data = NULL;
	//conn.start = 0;
	//conn.end = 0;
	conn.fd = tt->fd;

	// Set a timer for 30 seconds (eventually, set from cli or config or something)
	struct itimerspec tv = { 
		.it_value.tv_sec = 30,
		.it_value.tv_nsec = 0,
		.it_interval.tv_sec = 0,
		.it_interval.tv_sec = 0,
	};
	
  #if 1
	//You need to set a timer that will trigger and shut off when we reach a certain time
	int timer, status;
	timer_t timer_id = 0;
	struct sigevent evt = { 0 };
	evt.sigev_notify = SIGEV_THREAD;
	evt.sigev_notify_function = timer_expired;
	evt.sigev_value.sival_ptr = tt;

	if ( ( timer = timer_create( CLOCK_REALTIME, &evt, &timer_id ) ) != 0 ) {
		snprintf( conn.err, sizeof( conn.err ), "Error in timer_create: %s\n", strerror( errno ) );
		//pthread_exit( 0 );
		return 0;
	}

	// Start the timer
	if ( ( status = timer_settime( timer_id, 0, &tv, NULL )	) != 0 ) {
		snprintf( conn.err, sizeof( conn.err ), "Error in timer_settime: %s\n", strerror( errno ) );
		//pthread_exit( 0 );
		return 0;
	}
  #endif

	// Send a response
	if ( !srv_response( p, &conn ) ) {
		snprintf( conn.err, sizeof( conn.err ), "Error in TCP socket handling.\n" );
		FPRINTF( conn.err );
		//pthread_exit(0);
	}

	// Close a file
	if ( close( conn.fd ) == -1 ) {
		snprintf( conn.err, sizeof( conn.err ), "Error closing TCP socket connection: %s\n", strerror( errno ) );
		FPRINTF( conn.err );
	}

  #if 1
	//Delete the timer
	if ( timer_delete( timer_id ) == -1 ) {
		snprintf( conn.err, sizeof( conn.err ), "Error deleting timer: %s\n", strerror( errno ) );
		FPRINTF( conn.err );
		return 0;
	}
  #endif


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
			tt->running = 0;
			return 0;
		}
	}
  #endif
	FPRINTF( "Child process is exiting.\n" );
	tt->running = 0;

	// For right now, this will prevent high memory usage...
	//pthread_exit( 0 );
	return 0;
}



// A multithreaded server
//int srv_multithread( struct something_t *b, char *err, int errlen ) {
int srv_multithread( server_t *p ) {
	pthread_attr_t attr;


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

	for ( int fd = 0, count = 0; ; ) {
		//Client address and length?
		char ip[ 128 ] = { 0 };
		struct threadinfo_t *f = &_fds[ count ];
		struct sockaddr_storage addrinfo = { 0 };
		socklen_t addrlen = sizeof( addrinfo );
		
		//Set a reference to the server
		f->server = p;

		//Accept a new connection	
		if ( ( f->fd = accept( p->fd, (struct sockaddr *)&addrinfo, &addrlen ) ) == -1 ) {
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

		// all of the checks were successful, so add the context to threadinfo
		//f->ctx = p->ctx;

		//Log an access message including the IP in either ipv6 or v4
		if ( addrinfo.ss_family == AF_INET )
			inet_ntop( AF_INET, &((struct sockaddr_in *)&addrinfo)->sin_addr, ip, sizeof( ip ) ); 
		else {
			inet_ntop( AF_INET6, &((struct sockaddr_in6 *)&addrinfo)->sin6_addr, ip, sizeof( ip ) ); 
		}

		//Populate any other thread data
	#ifndef REAPING_THREADS 
		FPRINTF( "IP addr is: %s\n", ip );
	#else
		memcpy( f->ipaddr, ip, sizeof( ip ) );
		FPRINTF( "IP addr is: %s\n", ip );
	#endif

		//Start a new thread
		if ( pthread_create( &f->id, NULL, server_proc, f ) != 0 ) {
			FPRINTF( "Pthread create unsuccessful.\n" );
			continue;
		}

		FPRINTF( "Starting new thread.\n" );
	#ifndef REAPING_THREADS 
	#else
	#endif

	#if 0
		//This SHOULD work, but doesn't because there is no way to track whether or not it finished
		count++;
		if ( pthread_detach( f->id ) != 0 ) {
			FPRINTF( "Pthread detach unsuccessful.\n" );
			continue;
		}
	#else
		if ( ++count >= CLIENT_MAX_SIMULTANEOUS ) {
			for ( count = 0; count >= CLIENT_MAX_SIMULTANEOUS; count++ ) {
				if ( _fds[ count ].running ) {
					int s = pthread_join( _fds[ count ].id, NULL ); 
					FPRINTF( "Joining thread at pos %d, status = %d....\n", count, s );
					if ( s != 0 ) {
						FPRINTF( "Pthread join at pos %d failed...\n", count );
						continue;
					}
				}
			}
			count = 0;
		}
	#endif
		FPRINTF( "Waiting for next connection.\n" );
	}
}

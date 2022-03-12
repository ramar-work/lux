/* ------------------------------------------- * 
 * ctx-http.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTP contexts.
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "ctx-http.h"
#include <sys/sendfile.h>

//Size of \r\n\r\n
#define BHSIZE 4	

#if CTX_READ_SIZE < 4096
	#error "Context read size is too small."
#endif

//Define an interval for polling 
static const struct timespec __interval__ = { 0, POLL_INTERVAL };


//No-op
void create_notls ( void **p ) { ; }


static void dumpconn( struct cdata *conn, const char *fname ) {
	if ( conn ) {	

		FPRINTF( "SERVER INFO\n" );
		FPRINTF( "[%s] count: %d\n", fname, conn->count );
		FPRINTF( "[%s] status: %d\n", fname, conn->status );
		FPRINTF( "[%s] ipv4: %s\n", fname, conn->ipv4 );
		FPRINTF( "[%s] ipv6: %s\n", fname, conn->ipv6 );

		FPRINTF( "SERVER CONFIG\n" );
		FPRINTF( "[%s] ptr: %p\n", fname, conn->config );
		if ( conn->config ) {
			FPRINTF( "[%s] wwwroot: %s\n", fname, conn->config->wwwroot );
			FPRINTF( "[%s] hosts: %p\n", fname, conn->config->hosts );
		}

#if 0
		FPRINTF( "LOCAL CONFIG\n" );
		FPRINTF( "[%s] ptr : %p\n", fname, conn->hconfig );
		if ( conn->hconfig ) {
			FPRINTF( "[%s] name	: %s\n", fname, conn->hconfig->name	 );
			FPRINTF( "[%s] alias: %s\n", fname, conn->hconfig->alias );
			FPRINTF( "[%s] dir	: %s\n", fname, conn->hconfig->dir	 );
			FPRINTF( "[%s] filter	: %s\n", fname, conn->hconfig->filter	 );
			FPRINTF( "[%s] root_default	: %s\n", fname, conn->hconfig->root_default	 );
			FPRINTF( "[%s] ca_bundle: %s\n", fname, conn->hconfig->ca_bundle );
			FPRINTF( "[%s] cert_file: %s\n", fname, conn->hconfig->cert_file );
			FPRINTF( "[%s] key_file: %s\n", fname, conn->hconfig->key_file );
		}
#endif
	}
}


const int pre_notls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	return 1;
}


//Read a message that the server will use later.
const int read_notls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	FPRINTF( "Read started...\n" );

	//Get the time at the start
	int total = 0, nsize, mult = 1, size = CTX_READ_SIZE; 
	int hlen = 0, mlen = 0;
	struct timespec timer = {0};
	unsigned char *x = NULL, *xp = NULL;
	clock_gettime( CLOCK_REALTIME, &timer );	

#if 1
	//Set another pointer for just the headers
	memset( x = rq->preamble, 0, ZHTTP_PREAMBLE_SIZE );
#else
	//Allocate space for the first call
	if ( !( rq->msg = malloc( size ) ) || !memset( rq->msg, 0, size ) )
		return http_set_error( rs, 500, "Could not allocate initial read buffer." ); 

	if ( !memset( x = rq->msg, 0, size ) )
		return 0;
#endif

	//Read whatever the server sends and read until complete.
	for ( int rd, recvd = -1, bsize = ZHTTP_PREAMBLE_SIZE; recvd < 0;  ) {
		if ( ( rd = recv( fd, x, bsize, MSG_DONTWAIT ) ) == 0 ) {
			conn->count = -2; //most likely resources are unavailable
			return 0;		
		}
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );
			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				FPRINTF( "Got socket read error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				conn->count = -3;
				return http_set_error( rs, 408, "Timeout reached." );
			}

			//FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			FPRINTF( "Received %d additional header bytes on fd %d\n", rd, fd ); 
			bsize -= rd, total += rd, x += rd;
			recvd = http_header_received( rq->preamble, total );
			hlen = recvd; //rq->mlen = total;
		#if 0
			//rq->hlen = total - 4;
			FPRINTF( 
				"bsize: %d,"
				"total: %d,"
				"recvd: %d,"
				, bsize, total, recvd );
		#endif
		}
	}

	//This should probably be a while loop
	if ( !http_parse_header( rq, hlen ) ) {
		conn->count = -3;
		return http_set_error( rs, 500, (char *)rq->errmsg ); 
	}

	//If the message is not idempotent, stop and return.
	//print_httpbody( rq );

#if 0
	//Dump a message
	fprintf( stderr, "MESSAGE SO FAR:\n" );
	write( 2, rq->preamble, total );
#endif

	if ( !rq->idempotent ) {
		//rq->atype = ZHTTP_MESSAGE_STATIC;
		FPRINTF( "Read complete.\n" );
		return 1;
	}

#if 0
	fprintf( stderr, "%d ?= %d\n", rq->mlen, rq->hlen + 4 + rq->clen );
	conn->count = -3;
	return http_set_error( rs, 200, "OK" ); 
#endif

	//Check to see if we've fully received the message
	if ( total == ( hlen + BHSIZE + rq->clen ) ) {
		rq->msg = rq->preamble + ( hlen + BHSIZE );
		if ( !http_parse_content( rq, rq->msg, rq->clen ) ) {
			conn->count = -3;
			return http_set_error( rs, 500, (char *)rq->errmsg ); 
		}
		FPRINTF( "Read complete.\n" );
		return 1;
	}

	//Check here if the thing is too big
	if ( rq->clen > CTX_READ_MAX ) {
		conn->count = -3;
		char errmsg[ 1024 ] = {0};
		snprintf( errmsg, sizeof( errmsg ),
			"Content-Length (%d) exceeds read max (%d).", rq->clen, CTX_READ_MAX );
		return http_set_error( rs, 500, (char *)errmsg ); 
	} 	

	//Unsure if we still need this...
	#if 1
	if ( 1 )
		nsize = rq->clen;	
	#else
	if ( !rq->chunked )
		nsize = rq->mlen + rq->clen + 4;	
	else {
		//For chunked encoding, allocate a sensible size.
		//Then send a 100-continue to the server...
		nsize = rq->mlen + size + 4;
		char *a = http_make_request( rs, 100, "Continue" );
		send( a ); 
	}
	#endif

	//Allocate space for the content of the message (may wish to initialize the memory)
	rq->atype = ZHTTP_MESSAGE_MALLOC;
	if ( !( xp = rq->msg = malloc( nsize ) ) || !memset( xp, 0, nsize ) ) {
		conn->count = -3;
		return http_set_error( rs, 500, strerror( errno ) );
	}

	//Take any excess in the preamble and move that into xp
	int crecvd = total - ( hlen + BHSIZE );
	if ( crecvd > 0 ) {
		unsigned char *hp = rq->preamble + ( hlen + BHSIZE );
		memmove( xp, hp, crecvd );
		memset( hp, 0, crecvd );
		xp += crecvd; 
	} 

	//Get the rest of the message
	//FPRINTF( "crecvd: %d, clen: %d\n", crecvd, rq->clen );
	for ( int rd, bsize = size; crecvd < rq->clen; ) {
		FPRINTF( "Attempting read of %d bytes in ptr %p\n", bsize, xp );
		//FPRINTF( "crevd: %d, clen: %d\n", crecvd, rq->clen );
		if ( ( rd = recv( fd, xp, bsize, MSG_DONTWAIT ) ) == 0 ) {
			conn->count = -2; //most likely resources are unavailable
			return 0;		
		}
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				FPRINTF( "Got socket read error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				conn->count = -3;
				return http_set_error( rs, 408, "Timeout reached." );
			}

			FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			//Process a successfully read buffer
			FPRINTF( "Received %d additional bytes on fd %d\n", rd, fd ); 
			xp += rd, total += rd, crecvd += rd;
			if ( ( rq->clen - crecvd ) < size ) {
				bsize = rq->clen - crecvd;
			}

			//Set timer to keep track of long running requests
			FPRINTF( "Total so far: %d\n", total );
			clock_gettime( CLOCK_REALTIME, &timer );	
		}
	}

	//Finally, process the body (chunked may still need something fancy)
	if ( !http_parse_content( rq, rq->msg, rq->clen ) ) {
		conn->count = -3;
		return http_set_error( rs, 500, (char *)rq->errmsg ); 
	}

	FPRINTF( "Read complete (read %d out of %d bytes for content)\n", crecvd, rq->clen );
	return 1;
}


//Write
const int write_notls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	FPRINTF( "Write started...\n" );
	int sent = 0, pos = 0, try = 0, total = rs->mlen;
	unsigned char *ptr = rs->msg;

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );

	#if 1
	if ( rs->atype == ZHTTP_MESSAGE_SENDFILE ) {
		//Send the header first
		//for ( ;; ) {
			//
		int hlen = total;	
		for ( ; total; ) {
			sent = send( fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
			if ( sent == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;	
			}
			else if ( sent > -1 ) {
				pos += sent, total -= sent, ptr += sent;	
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				if ( total == 0 ) {
					FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
					break;
				}
			}
			else {
				if ( !total ) {
					FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
					return 1;
				}
				
				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
					conn->count = -2;
					return 0;	
				}

				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
					conn->count = -3;
					return http_set_error( rs, 408, "Timeout reached." );
				}

				FPRINTF("Trying again to send header to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Header Write complete (sent %d out of %d bytes)\n", pos, hlen );
		//}

		//Then send the file
		for ( total = rs->clen; total; ) {
			sent = sendfile( fd, rs->fd, NULL, CTX_WRITE_SIZE );
			FPRINTF( "Bytes sent from open file %d: %d\n", fd, sent );
			if ( sent == 0 ) 
				break;
			else if ( sent > -1 )
				total -= sent, pos += sent;
			else {
				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
					conn->count = -2;
					return 0;	
				}

				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
					conn->count = -3;
					return http_set_error( rs, 408, "Timeout reached." );
				}

				FPRINTF("Trying again to send file to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, rs->clen );
	}
	else {

	}
	#endif

	//Start writing data to socket
	for ( ;; ) {
		sent = send( fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
		FPRINTF( "Bytes sent: %d\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			if ( total == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;
			}
		}
		else {
			if ( !total ) {
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				return 1;
			}
			
			if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
				FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;	
			}

			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );
			
			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				conn->count = -3;
				return http_set_error( rs, 408, "Timeout reached." );
			}

			nanosleep( &__interval__, NULL );
		}
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}

	FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, rq->mlen );
	return 1;
}

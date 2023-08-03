/* ------------------------------------------- * 
 * ctx-http.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTP contexts.
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * ------------------------------------------- */
#include "ctx-http.h"

// Size of \r\n\r\n
static const int bhsize = 4;

// Size of zhttp_t object
static const int zhttp_size = sizeof( zhttp_t );

// Define an interval for polling 
static const struct timespec __interval__ = { 0, POLL_INTERVAL };



// Create an HTTPBody
static zhttp_t * create_zhttp_t ( HttpServiceType t ) {
	zhttp_t * z = NULL;

	if ( !( z = malloc( zhttp_size ) ) || !memset( z, 0, zhttp_size ) ) {
		return NULL;
	}

	z->type = t;
	return z;
}



// Does nothing, but is required to be a member of all contexts.
int create_notls ( server_t *p ) {
	return 1; 
}



// Also does nothing, but is required to be a member of all contexts.
void free_notls ( server_t *p ) {
	return; 
}



// Allocate these structures
const int pre_notls ( server_t *p, conn_t *conn ) {

	if ( !( conn->req = create_zhttp_t( ZHTTP_IS_CLIENT ) ) ) {
		FPRINTF( "(%s)->pre failure: %s\n", p->ctx->name, "HTTP read end init failed" );
		return 0;
	}

	if ( !( conn->res = create_zhttp_t( ZHTTP_IS_SERVER ) ) ) {
		FPRINTF( "(%s)->pre failure: %s\n", p->ctx->name, "HTTP write end init failed" );
		return 0;
	}

	return 1;
}



// Read a message that the server will use later.
const int read_notls ( server_t *p, conn_t *conn ) {

	// Get the time at the start
	int total = 0, nsize, mult = 1;
	int hlen = -1, mlen = 0;
	int bsize = ZHTTP_PREAMBLE_SIZE;
	const int size = CTX_READ_SIZE;
	struct timespec timer = {0};
	struct timespec n = {0};
	unsigned char *x = NULL, *xp = NULL;

	// Get the time
	clock_gettime( CLOCK_REALTIME, &timer );

	// Set another pointer for just the headers
	memset( x = conn->req->preamble, 0, ZHTTP_PREAMBLE_SIZE );

	// Read whatever the server sends and read until complete.
	for ( int rd, recvd = -1; recvd < 0 || bsize <= 0;  ) {
		rd = recv( conn->fd, x, bsize, MSG_DONTWAIT );
		if ( rd == 0 ) {
			// TODO: This indicates either an extremely slow read or perhaps a closed conn
			break;
		}
		else if ( rd < 1 ) {

			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				// TODO: This should be logged somewhere
				snprintf( conn->err, sizeof( conn->err ),
						"Got socket read error: %s\n", strerror( errno ) );
				FPRINTF( "FATAL: %s\n", conn->err );
				conn->stage = CONN_POST;
				return 0;
			}

			// Get the time
			memset( &n, 0, sizeof( struct timespec ) );
			clock_gettime( CLOCK_REALTIME, &n );

			// NOTE: This runs after an arbitrary limit
			// TODO: Need to analyze avg write size & make sure that it is "worth it"
			if ( ( n.tv_sec - timer.tv_sec ) >= p->rtimeout  ) {
				conn->stage = CONN_WRITE;
				(void)http_set_error( conn->res, 408, "Timeout reached." );
				return 1;
			}

			// FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			FPRINTF( "Received %d additional header bytes on fd %d\n", rd, conn->fd ); 
			bsize -= rd, total += rd, x += rd;
			recvd = http_header_received( conn->req->preamble, total ); 
			hlen = recvd;
			if ( recvd == ZHTTP_PREAMBLE_SIZE ) {
				break;
			}
			// FPRINTF( "bsize: %d, total: %d, recvd: %d,", bsize, total, recvd );
		}
	}

	// Stop if the header was just too big
	if ( hlen == -1 ) {
		conn->stage = CONN_WRITE;
		(void)http_set_error( conn->res, 500, "Header too large" );
		return 1;
	}

	// This should probably be a while loop
	if ( !http_parse_header( conn->req, hlen ) ) {
		conn->stage = CONN_WRITE;
		(void)http_set_error( conn->res, 500, (char *)conn->req->errmsg );
		return 1;
	}

	// If the message is not idempotent, stop and return.
	if ( !conn->req->idempotent ) {
		conn->stage = CONN_PROC;
		FPRINTF( "%s: Read complete, no content body (read %d bytes)\n",
				p->ctx->name, total );
		return 1;
	}

	// Check to see if we've fully received the message
	if ( total == ( hlen + bhsize + conn->req->clen ) ) {
		conn->req->msg = conn->req->preamble + ( hlen + bhsize );
		if ( !http_parse_content( conn->req, conn->req->msg, conn->req->clen ) ) {
			conn->stage = CONN_WRITE;
			(void)http_set_error( conn->res, 500, (char *)conn->req->errmsg );
			return 1;
		}
		FPRINTF( "%s: Read complete, finished parsing content body (read %d bytes)\n",
				p->ctx->name, total );
		conn->stage = CONN_PROC;
		return 1;
	}

	// Check here if the thing is too big
	if ( conn->req->clen > CTX_READ_MAX ) {
		snprintf( conn->err, sizeof( conn->err ),
			"Content-Length (%d) exceeds read max (%d).", conn->req->clen, CTX_READ_MAX );
		conn->stage = CONN_WRITE;
		(void)http_set_error( conn->res, 500, (char *)conn->err );
		return 1;
	}

	#if 1
	nsize = conn->req->clen;
	#else
	if ( !conn->req->chunked )
		nsize = conn->req->mlen + conn->req->clen + 4;	
	else {
		// For chunked encoding, allocate a sensible size.
		// Then send a 100-continue to the server...
		nsize = conn->req->mlen + size + 4;
		char *a = http_make_request( conn->res, 100, "Continue" );
		send( a ); 
	}
	#endif

	// Allocate space for the content of the message (may wish to initialize the memory)
	conn->req->atype = ZHTTP_MESSAGE_MALLOC;
	if ( !( xp = conn->req->msg = malloc( nsize ) ) || !memset( xp, 0, nsize ) ) {
		snprintf( conn->err, sizeof( conn->err ),
			"Request queue full: %s.", strerror( errno ) );
		conn->stage = CONN_WRITE;
		(void)http_set_error( conn->res, 500, conn->err );
		return 1;
	}

	// Take any excess in the preamble and move that into xp
	int crecvd = total - ( hlen + bhsize );
	FPRINTF( "crecvd: %d, %d, %d, %d\n", 
		crecvd, ( hlen + bhsize ), nsize, conn->req->clen );
	if ( crecvd > 0 ) {
		unsigned char *hp = conn->req->preamble + ( hlen + bhsize );
		memmove( xp, hp, crecvd );
		memset( hp, 0, crecvd );
		xp += crecvd; 
	} 

	// Get the rest of the message
	// FPRINTF( "crecvd: %d, clen: %d\n", crecvd, conn->req->clen );
	for ( int rd, bsize = size; crecvd < conn->req->clen; ) {
		FPRINTF( "Attempting read of %d bytes in ptr %p\n", bsize, xp );
		// FPRINTF( "crevd: %d, clen: %d\n", crecvd, conn->req->clen );
		if ( ( rd = recv( conn->fd, xp, bsize, MSG_DONTWAIT ) ) == 0 ) {
			// TODO: Properly handle this case
			conn->stage = CONN_PROC;
			return 1;
		}
		else if ( rd < 1 ) {

			// Most likely the other side is closed
			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				snprintf( conn->err, sizeof( conn->err ),
					"Got socket read error: %s\n", strerror( errno ) );
				FPRINTF( "FATAL: %s\n", conn->err );
				conn->stage = CONN_POST;
				return 0;
			}

			memset( &n, 0, sizeof( struct timespec ) );
			clock_gettime( CLOCK_REALTIME, &n );

			if ( ( n.tv_sec - timer.tv_sec ) >= p->rtimeout ) {
				conn->stage = CONN_WRITE;
				(void)http_set_error( conn->res, 408, "Timeout reached." );
				return 1;
			}

			FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			// Process a successfully read buffer
			FPRINTF( "Received %d additional bytes on fd %d\n", rd, conn->fd ); 
			xp += rd, total += rd, crecvd += rd;
			if ( ( conn->req->clen - crecvd ) < size ) {
				bsize = conn->req->clen - crecvd;
			}

			// Set timer to keep track of long running requests
			FPRINTF( "Total read so far: %d\n", total );
			clock_gettime( CLOCK_REALTIME, &timer );
		}
	}

	// Finally, process the body (chunked may still need something fancy)
	if ( !http_parse_content( conn->req, conn->req->msg, conn->req->clen ) ) {
		conn->stage = CONN_WRITE;
		(void)http_set_error( conn->res, 500, (char *)conn->req->errmsg );
		return 1;
	}

	FPRINTF( "Read complete (read %d out of %d bytes for content)\n",
		crecvd, conn->req->clen );
	conn->stage = CONN_PROC;
	return 1;
}



// Write a message to regular, unencrypted socket
const int write_notls ( server_t *p, conn_t *conn ) {

	// Define
	int sent = 0, pos = 0, try = 0, total = conn->res->mlen;
	unsigned char *ptr = conn->res->msg;
	struct timespec timer = {0}, n = {0};

	// Get the time at the start
	clock_gettime( CLOCK_REALTIME, &timer );

	// Mark the next stage
	conn->stage = CONN_POST;

#ifdef SENDFILE_ENABLED 
	if ( conn->res->atype == ZHTTP_MESSAGE_SENDFILE ) {
		// Send the header first
		int hlen = total;	
		for ( ; total; ) {
			sent = send( conn->fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
			if ( sent == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", conn->res->mlen );
				break;
			}
			else if ( sent > -1 ) {
				pos += sent, total -= sent, ptr += sent;
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				if ( total == 0 ) {
					FPRINTF( "sent == 0, assuming all %d bytes of header have been sent...\n", conn->res->mlen );
					break;
				}
			}
			else {
				if ( !total ) {
					FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
					return 1;
				}

				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					// Most likely, the other end closed early
					snprintf( conn->err, sizeof( conn->err ), 
						"Got socket write error: %s\n", strerror( errno ) );
					FPRINTF( "FATAL: %s\n", conn->err );
					conn->stage = CONN_POST;
					return 0;
				}

				memset( &n, 0, sizeof( struct timespec ) );	
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) >= p->wtimeout ) {
					// Cut if we can't get this message out for some reason
					snprintf( conn->err, sizeof( conn->err ), 
						"Timeout reached on write end of socket - header." );
					FPRINTF( "%s\n", conn->err );
					conn->stage = CONN_POST;
					return 0;
				}

				FPRINTF("Trying again to send header to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Header write complete (sent %d out of %d bytes)\n", pos, hlen );

		// Then send the file
		for ( total = conn->res->clen; total; ) {
			sent = sendfile( conn->fd, conn->res->fd, NULL, CTX_WRITE_SIZE );
			FPRINTF( "Bytes sent from open file %d: %d\n", conn->fd, sent );
			if ( sent == 0 )
				break;
			else if ( sent > -1 )
				total -= sent, pos += sent;
			else {
				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					snprintf( conn->err, sizeof( conn->err ),
						"Got socket write error: %s\n", strerror( errno ) );
					FPRINTF( "FATAL: %s\n", conn->err );
					conn->stage = CONN_POST;
					return 0;
				}

				memset( &n, 0, sizeof( struct timespec ) );
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > p->wtimeout ) {
					snprintf( conn->err, sizeof( conn->err ),
						"Timeout reached on write end of socket - body." );
					FPRINTF( "FATAL: %s\n", conn->err );
					conn->stage = CONN_POST;
					return 0;
				}

				FPRINTF("Trying again to send file to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, conn->res->clen );
		return 1;
	}
#endif

	// Start writing data to socket
	for ( ;; ) {
		sent = send( conn->fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
		FPRINTF( "Bytes sent: %d, over file %d\n", sent, conn->fd );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", conn->res->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			if ( total == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", conn->res->mlen );
				break;
			}
		}
		else {
			if ( !total ) {
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				return 1;
			}
			
			if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
				snprintf( conn->err, sizeof( conn->err ),
					"Got socket write error: %s\n", strerror( errno ) );
				FPRINTF( "FATAL: %s\n", conn->err );
				conn->stage = CONN_POST;
				return 0;	
			}

			memset( &n, 0, sizeof( struct timespec ) );	
			clock_gettime( CLOCK_REALTIME, &n );
			
			if ( ( n.tv_sec - timer.tv_sec ) > p->wtimeout ) {
				snprintf( conn->err, sizeof( conn->err ), 
					"Timeout reached on write end of socket - body." );
				FPRINTF( "%s\n", conn->err );
				conn->stage = CONN_POST;
				return 0;
			}

			nanosleep( &__interval__, NULL );
		}
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}

	FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, conn->req->mlen );
	return 1;
}



// Deallocate these structures
const void post_notls ( server_t *p, conn_t *conn ) {
	// Also need to destroy the http bodies
	http_free_body( conn->req ), http_free_body( conn->res );

	// Then free the structures
	free( conn->req ), free( conn->res );

	return;
}

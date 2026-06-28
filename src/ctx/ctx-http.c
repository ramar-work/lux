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

// 32 x 4 for now...
#define IOVEC_HEADER_MAX 128

// Size of \r\n\r\n
static const int bhsize = 4;

// Size of zhttp_t object
static const int zhttp_size = sizeof( zhttp_t );

//
static const char newline[] = "\r\n";

//
static const char prefmt[] = "%x\r\n";

// Define an interval for polling
static const struct timespec __interval__ = { 0, POLL_INTERVAL };

// Define a chunked encoding
static const char encchunked[] = "Transfer-Encoding: chunked\r\n";

// Close by default for now
static const char cclose[] = "Connection: close\r\n";


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
			"Content-Length (%lu) exceeds read max (%d).", conn->req->clen, CTX_READ_MAX );
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
	FPRINTF( "crecvd: %d, %d, %d, %lu\n",
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

	FPRINTF( "Read complete (read %d out of %ld bytes for content)\n",
		crecvd, conn->req->clen );
	conn->stage = CONN_PROC;
	return 1;
}



// Write a message to regular, unencrypted socket
const int write_notls ( server_t *p, conn_t *conn ) {

	// Define
	int sent = 0;
	int chunk = 0;
	int veccount = 1;

	unsigned int hlen = 0;
	unsigned long ptrlen = 0; //conn->res->mlen;
	unsigned char *ptr = 0; //conn->res->msg;

	time_t starttime = 0;
	zhttp_t *r = conn->res;
	zhttpr_t **hx = conn->res->headers;

	struct iovec vec[ IOVEC_HEADER_MAX ];
	struct timespec timer = {0};

	// Get the time at the start
	clock_gettime( CLOCK_REALTIME, &timer );
	starttime = timer.tv_sec;

	// Mark the next stage
	conn->stage = CONN_POST;

	// Set mesage pointers and sizes
	if ( r->atype == ZHTTP_MESSAGE_STATIC || r->atype == ZHTTP_MESSAGE_MALLOC ) {
		hlen = r->mlen - r->clen;
		//ptrlen = (*r->body)->size;
		//ptr = (*r->body)->value;
		ptr = r->msg + hlen;
		ptrlen = r->clen;
	}
	else if ( r->atype == ZHTTP_MESSAGE_SENDFILE ) {
		hlen = r->mlen;
		ptrlen = r->clen;
		ptr = mmap( NULL, ptrlen, PROT_READ, MAP_PRIVATE, r->fd, 0 );
		if ( ptr == MAP_FAILED ) {
			FPRINTF( "FATAL: mmap() failed: %s\n", strerror( errno ) );
			return 0;
		}
	}
	else {
		FPRINTF( "FATAL: Message preparation failed\n" );
		return 0;
	}

#if 0
	// If the client explicitly asked you not to chunk and the message is too big
	// you'll have to reject
#endif

	// Initialize the header
	memset( vec, 0, sizeof( struct iovec ) * IOVEC_HEADER_MAX );
	vec[ 0 ].iov_base = r->msg, vec[ 0 ].iov_len = hlen;

	#if 0	
	// Additional headers should be done from here too...
	for ( ; hx && *hx; hx++, hcount += 4 ) {
		if ( hcount >= IOVEC_HEADER_MAX - 1 || hcount > IOV_MAX ) {
			FPRINTF( "FATAL: max header count reached\n" );
			if ( r->atype == ZHTTP_MESSAGE_SENDFILE ) munmap( ptr, ptrlen );
			return 0;
		}
		i->iov_base = (void *)(*hx)->field, i->iov_len = strlen( (*hx)->field ), i++;	
		i->iov_base = (void *)hdiv, i->iov_len = 2, i++;	
		i->iov_base = (*hx)->value, i->iov_len = (*hx)->size, i++;	
		i->iov_base = (void *)hnewl, i->iov_len = 2, i++;	
	}
	#endif

	// Figure the whole chunked thing out...
	if ( ptrlen <= p->wbuffer && r->atype != ZHTTP_MESSAGE_SENDFILE ) {
		// TODO: This should not be...
		vec[ 1 ].iov_base = ptr;
		vec[ 1 ].iov_len = ptrlen;
		veccount++, ptrlen = 0; // NOTE: ptrlen == 0 means that the chunk loop won't run
	}
	else {
		chunk = 1;
		vec[ 0 ].iov_len -= 2;
		vec[ 1 ].iov_base = (void *)encchunked;
		vec[ 1 ].iov_len = sizeof( encchunked ) - 1;
		veccount++;
		vec[ 2 ].iov_base = (void *)newline;
		vec[ 2 ].iov_len = 2;
		veccount++;
	}

#if 0
FPRINTF( "%ld\n", r->clen );
FPRINTF( "vec 0: %p, %ld\n", vec[0].iov_base, vec[0].iov_len );
FPRINTF( "vec 1: %p, %ld\n", vec[1].iov_base, vec[1].iov_len );
writev( 2, vec, veccount );
#endif

	// Decide whether or not to chunk
	if ( ( sent = writev( conn->fd, vec, veccount ) ) == -1 ) {
		FPRINTF( "FATAL: header write() failed: %s\n", strerror( errno ) );
		return 0;
	}


	// Send remainder of response via chunked encoding
	const unsigned long totlen = ptrlen;
	for ( int ssent = 0, buflen = p->wbuffer; ptrlen; ) {

		// Figure out before everything, how much is left, bin should always end at zero...
		if ( buflen > ptrlen ) {
			buflen = ptrlen;
		}	

		// Write the octal number and preamble
		char pre[16] = {0};
		int prelen = snprintf( pre, sizeof( pre ), prefmt, buflen );
		struct iovec v[3] = {
			{ .iov_base = pre, .iov_len = prelen },
			{ .iov_base = ptr, .iov_len = buflen },
			{ .iov_base = "\r\n", .iov_len = 2 },
		};	

		if ( ( ssent = writev( conn->fd, v, 3 ) ) == -1 ) {
			snprintf( conn->err, sizeof( conn->err ), "Got socket write error: %s\n", strerror( errno ) );
			FPRINTF( "FATAL: %s\n", conn->err );
			conn->stage = CONN_POST;
			return 0;
		}

		//writev( 2, v, 3 );
		//FPRINTF( "SENT = %d, REMAINING = %lu/%lu\n", ssent, ptrlen, totlen );

		// Increment binary
		sent += ssent, ptr += buflen, ptrlen -= buflen;
	}

	if ( chunk && write( conn->fd, "0\r\n\r\n", 5 ) == -1 ) {
		if ( errno == ENOBUFS || errno == ENOMEM )
			;/* TODO: We can save the message in these cases, but that's a later date */
		snprintf( conn->err, sizeof( conn->err ), "Got socket write error: %s\n", strerror( errno ) );
		FPRINTF( "FATAL: %s\n", conn->err );
		conn->stage = CONN_POST;
		return 0;
	}

	// This is at the end
	if ( r->atype == ZHTTP_MESSAGE_SENDFILE ) {
		munmap( ptr, ptrlen );
	}

	FPRINTF( "SENT %d/%lu BYTES TO TLS CONNECTION ID: %d\n", sent, totlen, conn->connid );
	return 1;
}



// Deallocate these structures
void post_notls ( server_t *p, conn_t *conn ) {
	// Also need to destroy the http bodies
	http_free_body( conn->req ), http_free_body( conn->res );

	// Then free the structures
	free( conn->req ), free( conn->res );

	return;
}

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


#ifdef DEBUG_H
// Dump all of the hosts that are currently loaded via the config
static void dump_hosts_list ( struct sconfig *conf ) {
	for ( struct lconfig **h = conf->hosts; *h ; h++ ) {
		FPRINTF( "Host %s contains:\n", (*h)->name );
		FPRINTF( "name: %s\n", (*h)->name );	
		FPRINTF( "alias: %s\n", (*h)->alias );
		FPRINTF( "dir: %s\n", (*h)->dir );	
		FPRINTF( "filter: %s\n", (*h)->filter );	
		FPRINTF( "root_default: %s\n", (*h)->root_default );	
		//FPRINTF( "ca_bundle: %s\n", (*h)->ca_bundle );
		FPRINTF( "tls ready: %d\n", (*h)->tlsready );
		FPRINTF( "cert_file: %s\n", (*h)->cert_file );
		FPRINTF( "key_file: %s\n", (*h)->key_file );
	}
}
#endif


//Define an interval for polling 
static const struct timespec __interval__ = { 0, POLL_INTERVAL };



// No-op
//int create_notls ( void **p, char *err, int errlen ) { 
int create_notls ( server_t *p ) {
	return 1; 
}



//
const int pre_notls ( server_t *p, conn_t *c ) {
	return 1;
}



// Read a message that the server will use later.
//const int read_notls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
const int read_notls ( server_t *p, conn_t *c ) {
#if 1
	FPRINTF( "Read started...\n" );

	//Get the time at the start
	int total = 0, nsize, mult = 1, size = CTX_READ_SIZE; 
	int hlen = -1, mlen = 0, bsize = ZHTTP_PREAMBLE_SIZE;
	unsigned char *x = NULL, *xp = NULL;

	//Get the time
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Set another pointer for just the headers
	memset( x = c->req->preamble, 0, ZHTTP_PREAMBLE_SIZE );

	//Read whatever the server sends and read until complete.
	for ( int rd, recvd = -1; recvd < 0 || bsize <= 0;  ) {
		rd = recv( c->fd, x, bsize, MSG_DONTWAIT ); 
		if ( rd == 0 ) {
			//c->count = -2; //most likely resources are unavailable
			break;		
		} 
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );
			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				FPRINTF( "Got socket read error: %s\n", strerror( errno ) );
				c->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				c->count = -3;
				return http_set_error( c->res, 408, "Timeout reached." );
			}

			//FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			FPRINTF( "Received %d additional header bytes on fd %d\n", rd, c->fd ); 
			bsize -= rd, total += rd, x += rd;
			recvd = http_header_received( c->req->preamble, total ); 
			hlen = recvd; //c->req->mlen = total;
			if ( recvd == ZHTTP_PREAMBLE_SIZE ) {
				
				FPRINTF( "At end of buffer...\n" );
				break;	
			}	
		#if 0
			//c->req->hlen = total - 4;
			FPRINTF( 
				"bsize: %d,"
				"total: %d,"
				"recvd: %d,"
				, bsize, total, recvd );
		#endif
		}
	}

#if 0
	//Dump a message
	fprintf( stderr, "MESSAGE SO FAR:\n" );
	fprintf( stderr, "HEADER LENGTH %d\n", hlen );
	write( 2, c->req->preamble, total );
#endif

	//Stop if the header was just too big
	if ( hlen == -1 ) {
		c->count = -3;
		return http_set_error( c->res, 500, "Header too large" ); 
	}

	//This should probably be a while loop
	if ( !http_parse_header( c->req, hlen ) ) {
		c->count = -3;
		return http_set_error( c->res, 500, (char *)c->req->errmsg ); 
	}

	//If the message is not idempotent, stop and return.
	//print_httpbody( rq );
	if ( !c->req->idempotent ) {
		//c->req->atype = ZHTTP_MESSAGE_STATIC;
print_httpbody( c->req );
		FPRINTF( "Read complete.\n" );
		return 1;
	}

#if 0
	fprintf( stderr, "%d ?= %d\n", c->req->mlen, c->req->hlen + 4 + c->req->clen );
	c->count = -3;
	return http_set_error( rs, 200, "OK" ); 
#endif
#if 0
	write( 2, c->req->preamble, total );
	write( 2, "\n", 1 );
	write( 2, "\n", 1 );
#endif

	//Check to see if we've fully received the message
	if ( total == ( hlen + BHSIZE + c->req->clen ) ) {
		c->req->msg = c->req->preamble + ( hlen + BHSIZE );
		if ( !http_parse_content( c->req, c->req->msg, c->req->clen ) ) {
			c->count = -3;
			return http_set_error( c->res, 500, (char *)c->req->errmsg ); 
		}
		//print_httpbody( c->req );
		FPRINTF( "Read complete.\n" );
		return 1;
	}

	//Check here if the thing is too big
	if ( c->req->clen > CTX_READ_MAX ) {
		c->count = -3;
		char errmsg[ 1024 ] = {0};
		snprintf( errmsg, sizeof( errmsg ),
			"Content-Length (%d) exceeds read max (%d).", c->req->clen, CTX_READ_MAX );
		return http_set_error( c->res, 500, (char *)errmsg ); 
	} 	

	//Unsure if we still need this...
	#if 1
	if ( 1 )
		nsize = c->req->clen;	
	#else
	if ( !c->req->chunked )
		nsize = c->req->mlen + c->req->clen + 4;	
	else {
		//For chunked encoding, allocate a sensible size.
		//Then send a 100-continue to the server...
		nsize = c->req->mlen + size + 4;
		char *a = http_make_request( c->res, 100, "Continue" );
		send( a ); 
	}
	#endif

	//Allocate space for the content of the message (may wish to initialize the memory)
	c->req->atype = ZHTTP_MESSAGE_MALLOC;
	if ( !( xp = c->req->msg = malloc( nsize ) ) || !memset( xp, 0, nsize ) ) {
		c->count = -3;
		return http_set_error( c->res, 500, strerror( errno ) );
	}

	//Take any excess in the preamble and move that into xp
	int crecvd = total - ( hlen + BHSIZE );
	FPRINTF( "crecvd: %d, %d, %d, %d\n", 
		crecvd, ( hlen + BHSIZE ), nsize, c->req->clen );
	if ( crecvd > 0 ) {
		unsigned char *hp = c->req->preamble + ( hlen + BHSIZE );
		memmove( xp, hp, crecvd );
		memset( hp, 0, crecvd );
		xp += crecvd; 
	} 

	//Get the rest of the message
	//FPRINTF( "crecvd: %d, clen: %d\n", crecvd, c->req->clen );
	for ( int rd, bsize = size; crecvd < c->req->clen; ) {
		FPRINTF( "Attempting read of %d bytes in ptr %p\n", bsize, xp );
		//FPRINTF( "crevd: %d, clen: %d\n", crecvd, c->req->clen );
		if ( ( rd = recv( c->fd, xp, bsize, MSG_DONTWAIT ) ) == 0 ) {
			c->count = -2; //most likely resources are unavailable
			return 0;		
		}
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				FPRINTF( "Got socket read error: %s\n", strerror( errno ) );
				c->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				c->count = -3;
				return http_set_error( c->res, 408, "Timeout reached." );
			}

			FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			//Process a successfully read buffer
			FPRINTF( "Received %d additional bytes on fd %d\n", rd, c->fd ); 
			xp += rd, total += rd, crecvd += rd;
			if ( ( c->req->clen - crecvd ) < size ) {
				bsize = c->req->clen - crecvd;
			}

			//Set timer to keep track of long running requests
			FPRINTF( "Total so far: %d\n", total );
			clock_gettime( CLOCK_REALTIME, &timer );	
		}
	}

#if 0
	FPRINTF( "MESSAGE CONTENTS\n" );
	write( 2, c->req->msg, crecvd );
	write( 2, "\n", 1 );
	write( 2, "\n", 1 );
#endif

	//Finally, process the body (chunked may still need something fancy)
	if ( !http_parse_content( c->req, c->req->msg, c->req->clen ) ) {
		c->count = -3;
		return http_set_error( c->res, 500, (char *)c->req->errmsg ); 
	}

	FPRINTF( "Read complete (read %d out of %d bytes for content)\n", crecvd, c->req->clen );
#endif
print_httpbody( c->req );
	return 1;
}



// Write a message to regular, unencrypted socket
//const int write_notls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
const int write_notls ( server_t *p, conn_t *c ) {
#if 1
	FPRINTF( "Write started...\n" );
	int sent = 0, pos = 0, try = 0, total = c->res->mlen;
	unsigned char *ptr = c->res->msg;

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );

     #ifdef SENDFILE_ENABLED 
	if ( c->res->atype == ZHTTP_MESSAGE_SENDFILE ) {
		//Send the header first
		int hlen = total;	
		for ( ; total; ) {
			sent = send( c->fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
			if ( sent == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", c->res->mlen );
				break;	
			}
			else if ( sent > -1 ) {
				pos += sent, total -= sent, ptr += sent;	
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				if ( total == 0 ) {
					FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", c->res->mlen );
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
					c->count = -2;
					return 0;	
				}

				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
					c->count = -3;
					return http_set_error( c->res, 408, "Timeout reached." );
				}

				FPRINTF("Trying again to send header to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Header Write complete (sent %d out of %d bytes)\n", pos, hlen );
		//}

		//Then send the file
		for ( total = c->res->clen; total; ) {
			sent = sendfile( c->fd, c->res->fd, NULL, CTX_WRITE_SIZE );
			FPRINTF( "Bytes sent from open file %d: %d\n", c->fd, sent );
			if ( sent == 0 ) 
				break;
			else if ( sent > -1 )
				total -= sent, pos += sent;
			else {
				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
					c->count = -2;
					return 0;	
				}

				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
					c->count = -3;
					return http_set_error( c->res, 408, "Timeout reached." );
				}

				FPRINTF("Trying again to send file to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, c->res->clen );
		return 1;
	}
     #endif

	//Start writing data to socket
	for ( ;; ) {
		sent = send( c->fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
		FPRINTF( "Bytes sent: %d\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", c->res->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			if ( total == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", c->res->mlen );
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
				c->count = -2;
				return 0;	
			}

			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );
			
			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				c->count = -3;
				return http_set_error( c->res, 408, "Timeout reached." );
			}

			nanosleep( &__interval__, NULL );
		}
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}

	FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, c->req->mlen );
#endif
	return 1;
}

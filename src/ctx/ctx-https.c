/* ------------------------------------------- * 
 * ctx-https.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTPS contexts.
 * Requires GnuTLS.
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - Added interval for fake polling.
 *  
 * ------------------------------------------- */
#include "ctx-https.h"
#include <sys/sendfile.h>

#ifndef DISABLE_TLS

//Size of \r\n\r\n
const unsigned short bhsize = 4;

//Interval for fake polling here...
static const struct timespec __interval__ = { 0, 100000000 };

void create_gnutls( void **p ) {
	if ( !gnutls_global_init() ) {
		
	}
	//gnutls_global_set_log_function( tls_log_func );
}


void free_gnults( void **p ) {
	gnutls_global_deinit();
}

void destroy_gnutls ( struct gnutls_abstr *g ) {
	gnutls_deinit( g->session );
	gnutls_priority_deinit( g->priority_cache );
	gnutls_certificate_free_credentials( g->x509_cred );
	free( g );
}


const int post_gnutls ( int fd, zhttp_t *a, zhttp_t *b, struct cdata *conn) {
#if 0
	struct gnutls_abstr *g = (struct gnutls_abstr *)*p;
	destroy_gnutls( g );
#endif
	return 0;
}



//This should be done one time before anything starts running
//Also meaning that global initialization and de-initialization are needed 
static int process_credentials ( struct gnutls_abstr *g, struct sconfig *conf ) {
#if 0
	//FPRINTF( "Runnnig proc credentials with hosts: %p\n", hosts );
	for ( struct lconfig **h = conf->hosts; *h ; h++ ) {
		FPRINTF( "Host %s contains:\n", (*h)->name );
		FPRINTF( "name: %s\n", (*h)->name );	
		FPRINTF( "alias: %s\n", (*h)->alias );
		FPRINTF( "dir: %s\n", (*h)->dir );	
		FPRINTF( "filter: %s\n", (*h)->filter );	
		FPRINTF( "root_default: %s\n", (*h)->root_default );	
		FPRINTF( "ca_bundle: %s\n", (*h)->ca_bundle );
		FPRINTF( "cert_file: %s\n", (*h)->cert_file );
		FPRINTF( "key_file: %s\n", (*h)->key_file );
	}
#endif
	for ( struct lconfig **h = conf->hosts; h && *h ; h++ ) {
		FPRINTF( "Host %s will init TLS: %s\n", (*h)->name, ( (*h)->dir && (*h)->cert_file ) ? "Y" : "N" );
		if ( (*h)->dir && (*h)->cert_file  ) {
			//Make a filename
			char cert[2048] = {0}, key[2048] = {0}, ca[2048] = {0};
		#if 0
		#else
			snprintf( cert, sizeof(cert), "%s/%s/%s", conf->wwwroot, (*h)->dir, (*h)->cert_file );
			snprintf( key, sizeof(key), "%s/%s/%s", conf->wwwroot, (*h)->dir, (*h)->key_file );
			snprintf( ca, sizeof(ca), "%s/%s/%s", conf->wwwroot, (*h)->dir, (*h)->ca_bundle );
			FPRINTF( "Attempting to process these certs\n" );
			FPRINTF( "ca bundle: %s\n", ca );
			FPRINTF( "cert: %s\n", cert );
			FPRINTF( "key: %s\n", key );
			char *w[] = { cert, key, /*ca, */NULL };
			//char *w[] = { cert, key, ca, NULL };
			for ( char **ww = w; *ww; ww++ ) {
				struct stat sb = {0};
				//Check that the file exists and is a regular file
				if ( stat( *ww, &sb ) == -1 ) {
					FPRINTF( "STAT ERR: %s\n", strerror( errno ) );
					return 0;	
				}
				
				if ( !S_ISREG( sb.st_mode ) ) {
					FPRINTF( "Cert part is not a regular file: %s\n", *ww );
					return 0;
				}
			}
		#endif
			//Defined here, but nice if this is not the case...
			int cp=0, status=0;
#if 0
			//TODO: This should match the number of TLS enabled clients
			if ( ( cp = gnutls_certificate_set_x509_trust_file( g->x509_cred, ca, GNUTLS_X509_FMT_PEM ) ) < 0 ) {
				FPRINTF( "Could not set trust for '%s': %s\n", (*h)->name, gnutls_strerror( cp ) );
				return 0;
			}
			FPRINTF( "Certificates processed: %d\n", cp );
#endif

			if ( ( status = gnutls_certificate_set_x509_key_file( g->x509_cred, cert, key, GNUTLS_X509_FMT_PEM ) ) < 0 ) {
				FPRINTF( "Could not set certificate for '%s': %s\n", (*h)->name, gnutls_strerror( status ) );
				return 0;
			}
			FPRINTF( "TLS connection data for host '%s' successfully initialized\n", (*h)->name );
		}
	}	
	return 1;
}


const int 
pre_gnutls ( int fd, zhttp_t *a, zhttp_t *b, struct cdata *conn ) {
	//The hosts SHOULD be here
	//So the SNI lookup can happen here...

	//Define
	struct gnutls_abstr *g = NULL;
	int ret, size = sizeof( struct gnutls_abstr );
	FPRINTF( "Starting PRE GnuTLS\n" );

	//Allocate a structure for the request process
	if ( !( g = malloc( size )) || !memset( g, 0, size ) ) { 
		FPRINTF( "Failed to allocate space for gnutls_abstr\n" );
		return 0;
	}

	//Allocate space for credentials
	ret = gnutls_certificate_allocate_credentials( &g->x509_cred );
	if ( ret != GNUTLS_E_SUCCESS ) {
		FPRINTF( "Failed to allocate GnuTLS credentials structure\n" );
		return 0;
	}

	//Get config hosts, and get all the key details
	if ( !process_credentials( g, conn->config ) ) {
		gnutls_certificate_free_credentials( g->x509_cred );
		free( g );
		FPRINTF( "Proc cred failure.\n" );
		return 0;
	}

	//Set up the rest of the credential data
	g->priority_cache = NULL;
	ret = gnutls_priority_init( &g->priority_cache, NULL, NULL );
	if ( ret != GNUTLS_E_SUCCESS ) {
		FPRINTF( "Failed to set priority: %s\n", gnutls_strerror( ret ) );
		return 0;
	}	

	ret = gnutls_init( &g->session, GNUTLS_SERVER | GNUTLS_NONBLOCK );
	if ( ret != GNUTLS_E_SUCCESS ) {
		FPRINTF( "Failed to initialize new TLS session: %s\n", gnutls_strerror( ret ) );
		return 0;
	}
	
	ret = gnutls_priority_set( g->session, g->priority_cache );
	if ( ret != GNUTLS_E_SUCCESS ) {
		FPRINTF( "Failed to set priority: %s\n", gnutls_strerror( ret ) );
		return 0;
	}
	
	ret = gnutls_credentials_set( g->session, GNUTLS_CRD_CERTIFICATE, g->x509_cred );
	if ( ret != GNUTLS_E_SUCCESS ) {
		FPRINTF( "Failed to set credentials: %s\n", gnutls_strerror( ret ) );
		return 0;
	}

	//Set a few more details for our current session.	
	gnutls_certificate_server_set_request( g->session, GNUTLS_CERT_IGNORE ); 
	gnutls_handshake_set_timeout( g->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
	gnutls_transport_set_int( g->session, fd );

	//Try to get the server name here?
	//Do the actual handshake with an open file descriptor
#if 1
	while ( ( ret = gnutls_handshake( g->session ) ) == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );
#else
	do {
		ret = gnutls_handshake( g->session );

		//It looks like GnuTLS does this in the background
		//It MIGHT be helpful to check that host and data match
	#ifndef DISABLE_SNI
		//char data[4096] = {0};
		unsigned int sni_type = GNUTLS_NAME_DNS;
		size_t dsize = sizeof( g->sniname ) - 1;	
		if ( ( ret = gnutls_server_name_get( g->session, g->sniname, &dsize, &sni_type, 0 ) ) < 0 ) {
			fprintf( stderr, "Could not get server name: %s\n", gnutls_strerror( ret ) );
			//return 0;
			//If the client does not use sni, then we should probably stop
		}
	#if 0
		//Check against host data, and perhaps send the cert to the client for check?
		if ( 0 ) {
			//should this result in a 404?
			for ( struct lconfig **h = conf->hosts; h && *h ; h++ ) {
				if ( !strcmp( data, (*h)->name ) || !strcmp( data, (*h)->alias ) ) {

				}
			}
		}
	#endif		
	#endif
	}
	while ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );	
#endif
	if ( ret < 0 ) {
		destroy_gnutls( g );
		FPRINTF( "GnuTLS handshake failed: %s\n", gnutls_strerror( ret ) );
		//This isn't a fatal error... but what do I return?
conn->count = -2;
		return 0;	
	}

	FPRINTF( "GnuTLS handshake succeeded.\n" );
	conn->ctx->data = g;
	return 1;
}



//Read a message that the server will use later.
const int read_gnutls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	FPRINTF( "Read started...\n" );

	//Set references, initialize pointers
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->ctx->data;
	int total = 0, nsize, mult = 1, size = CTX_READ_SIZE; 
	int hlen = -1, mlen = 0, bsize = ZHTTP_PREAMBLE_SIZE;
	unsigned char *x = NULL, *xp = NULL;

	//Get the time
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Bad certs can leave us with this sorry state
	if ( !g || !g->session ) {
		//set conn->count to stop here...
		return http_set_error( rs, 500, "TLS/TLS handshake error encountered." ); 
	}

	//Set another pointer for just the headers
	memset( x = rq->preamble, 0, ZHTTP_PREAMBLE_SIZE );

	//Read whatever the server sends and read until complete.
	for ( int rd, flags, recvd = -1; recvd < 0 || bsize <= 0;  ) {

	#if 0
		if ( ( rd = gnutls_record_recv( g->session, x, bsize ) ) == 0 ) {
			break;
		}
	#else
		rd = gnutls_record_recv( g->session, x, bsize ); 
		if ( rd == 0 ) {
			//conn->count = -2; //most likely resources are unavailable
			//TODO: May need to tear down the connection.
			break;		
		} 
	#endif
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			//Handle any TLS/TLS errors
			if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				FPRINTF( "TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				FPRINTF( "TLS got handshake reauthentication request...\n" );
				continue;	
			}
			else {
				FPRINTF( "TLS got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
				conn->count = -2;
				return http_set_error( rs, 500, (char *)gnutls_strerror( rd ) );
			}

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
			int pending = 0;
			bsize -= rd, total += rd, x += rd;
			recvd = http_header_received( rq->preamble, total ); 
			hlen = recvd; //rq->mlen = total;
			if ( recvd == ZHTTP_PREAMBLE_SIZE ) {
				FPRINTF( "At end of buffer...\n" );
				break;	
			}	
		#if 0
			//rq->hlen = total - 4;
			FPRINTF( 
				"bsize: %d,"
				"total: %d,"
				"recvd: %d,"
				, bsize, total, recvd );
		#endif
			if ( ( pending = gnutls_record_check_pending( g->session ) ) == 0 ) {
				FPRINTF( "No data left in buffer?\n" );
				break;
			}
			else {
				FPRINTF( "Data left in buffer: %d bytes\n", pending );
				continue;
			}
		}
	}

#if 0
	//Dump a message
	fprintf( stderr, "MESSAGE SO FAR:\n" );
	fprintf( stderr, "HEADER LENGTH %d\n", hlen );
	write( 2, rq->preamble, total );
#endif

	//Stop if the header was just too big
	if ( hlen == -1 ) {
		conn->count = -3;
		return http_set_error( rs, 500, "Header too large" ); 
	}

	//This should probably be a while loop
	if ( !http_parse_header( rq, hlen ) ) {
		conn->count = -3;
		return http_set_error( rs, 500, (char *)rq->errmsg ); 
	}

	//If the message is not idempotent, stop and return.
	print_httpbody( rq );
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
#if 0
	write( 2, rq->preamble, total );
	write( 2, "\n", 1 );
	write( 2, "\n", 1 );
#endif

	//Check to see if we've fully received the message
	if ( total == ( hlen + bhsize + rq->clen ) ) {
		rq->msg = rq->preamble + ( hlen + bhsize );
		if ( !http_parse_content( rq, rq->msg, rq->clen ) ) {
			conn->count = -3;
			return http_set_error( rs, 500, (char *)rq->errmsg ); 
		}
		//print_httpbody( rq );
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
	int crecvd = total - ( hlen + bhsize );
	FPRINTF( "crecvd: %d, %d, %d, %d\n", 
		crecvd, ( hlen + bhsize ), nsize, rq->clen );
	if ( crecvd > 0 ) {
		unsigned char *hp = rq->preamble + ( hlen + bhsize );
		memmove( xp, hp, crecvd );
		memset( hp, 0, crecvd );
		xp += crecvd; 
	} 

	//Get the rest of the message
	//FPRINTF( "crecvd: %d, clen: %d\n", crecvd, rq->clen );
	for ( int rd, bsize = size; crecvd < rq->clen; ) {
		FPRINTF( "Attempting read of %d bytes in ptr %p\n", bsize, xp );
		//FPRINTF( "crevd: %d, clen: %d\n", crecvd, rq->clen );
	#if 0
		if ( ( rd = gnutls_record_recv( g->session, xp, bsize ) ) == 0 ) {
		}
	#else
		rd = gnutls_record_recv( g->session, xp, bsize ); 
		if ( rd == 0 ) {
			conn->count = -2; //most likely resources are unavailable
			return 0;		
		}
	#endif
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			//Handle any TLS/TLS errors
			if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				FPRINTF( "TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				FPRINTF( "TLS got handshake reauth request...\n" );
				continue;	
			}
			else {
				FPRINTF( "TLS got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
				conn->count = -2;
				return http_set_error( rs, 500, (char *)gnutls_strerror( rd ) );
			}
			
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

#if 0
	FPRINTF( "MESSAGE CONTENTS\n" );
	write( 2, rq->msg, crecvd );
	write( 2, "\n", 1 );
	write( 2, "\n", 1 );
#endif

	//Finally, process the body (chunked may still need something fancy)
	if ( !http_parse_content( rq, rq->msg, rq->clen ) ) {
		conn->count = -3;
		return http_set_error( rs, 500, (char *)rq->errmsg ); 
	}

	FPRINTF( "Read complete (read %d out of %d bytes for content)\n", crecvd, rq->clen );
	return 1;
}


#if 0
const int read_gnutls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	FPRINTF( "Read started...\n" );

	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->ctx->data;
	int total = 0, nsize, mult = 1, size = CTX_READ_SIZE; 
	int hlen = -1, mlen = 0, bsize = ZHTTP_PREAMBLE_SIZE;
	unsigned char *x = NULL, *xp = NULL;

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Bad certs can leave us with this sorry state
	if ( !g || !g->session ) {
		//set conn->count to stop here...
		return http_set_error( rs, 500, "TLS/TLS handshake error encountered." ); 
	}

	//Prepare buffer for reading the first part of the message 
	memset( x = rq->preamble, 0, ZHTTP_PREAMBLE_SIZE );

	//Read first
	for ( ;; ) {	

		int flags, rd, pending, nsize = size * mult;
		unsigned char *ptr = rq->msg;
		ptr += ( nsize - size );

		if ( ( rd = gnutls_record_recv( g->session, ptr, size ) ) == 0 ) {
			//MSG_DONTWAIT - How do I emulate this behavior?
			conn->count = -2; //most likely resources are unavailable
			return 0;		
		}
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			//Handle any TLS/TLS errors
			if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				FPRINTF( "TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				FPRINTF( "TLS got handshake reauth request...\n" );
				continue;	
			}
			else {
				FPRINTF( "TLS got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
				conn->count = -2;
				return http_set_error( rs, 500, (char *)gnutls_strerror( rd ) );
			}
			
			//Then handle any socket errors
			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				conn->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				conn->count = -3;
				return http_set_error( rs, 408, "Timeout reached." );
			}

			FPRINTF( "Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			rq->mlen += rd;
			zhttp_t *tmp = http_parse_request( rq, err, sizeof(err) ); 
	#if 0
			//Check that the hostname matches the SNI name
			if ( strcmp( tmp->host, g->sniname ) ) {
				conn->count = -3;
				snprintf( err, sizeof( err ), "Requested cert host '%s' does not match hostname '%s'.", tmp->host, g->sniname ); 
				return http_set_error( rs, 500, err );
			}
	#endif
	
			//TODO: Is this handling everything?
			if ( tmp->error == ZHTTP_NONE ) { 
				FPRINTF( "All data received\n" );
				break;
			}
			else if ( tmp->error != ZHTTP_INCOMPLETE_HEADER ) { // && tmp->error !=	ZHTTP_INCOMPLETE_REQUEST
				FPRINTF( "Got fatal HTTP parser error: %s\n", err );
				conn->count = -3;
				return http_set_error( rs, 500, err ); 
			}

			if ( !( rq->msg = realloc( rq->msg, nsize ) ) || !memset( &rq->msg[ nsize - size ], 0, size ) ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			if ( ( pending = gnutls_record_check_pending( g->session ) ) == 0 ) {
				FPRINTF( "No data left in buffer?\n" );
				break;
			}
			else {
				FPRINTF( "Data left in buffer: %d bytes\n", pending );
				continue;
			}

			FPRINTF( "Received %d bytes on fd %d\n", rd, fd ); 
			clock_gettime( CLOCK_REALTIME, &timer );	
			mult++;
		}
	}
	FPRINTF( "Read complete.\n" );
	return 1;
}
#endif


//Write
const int write_gnutls (int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn) {
	FPRINTF( "Write started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->ctx->data;
	int sent = 0, pos = 0, try = 0, total = rs->mlen, pending;
	unsigned char *ptr = rs->msg;

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );

	#if 0
	if ( rs->atype == ZHTTP_MESSAGE_SENDFILE ) {
		//Send the header first
		int hlen = total;	
		for ( ; total; ) {
			//sent = send( fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
			sent = gnutls_record_send( g->session, ptr, total );
			if ( sent == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;
			}
			else if ( sent > -1 ) {
				pos += sent, total -= sent, ptr += sent;	
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				int pending = gnutls_record_check_pending( g->session ); 
				FPRINTF( "pending == %d, do we try again?\n", pending );
				if ( total == 0 ) {
					FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
					break;
				}
			}
			else {
				if ( sent != GNUTLS_E_INTERRUPTED || sent != GNUTLS_E_AGAIN || errno != EAGAIN || errno != EWOULDBLOCK ) {
					//This is some uncaught condition, probably one of these: 
					//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
					FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
					conn->count = -2;
					return 0;
				}

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
			//sent = sendfile( fd, rs->fd, NULL, CTX_WRITE_SIZE );
			//TODO: Test this extensively, g->session should hold an open file
			sent = gnutls_record_send_file( g->session, rs->fd, NULL, CTX_WRITE_SIZE );
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
		//sent = send( fd, ptr, total, MSG_DONTWAIT | MSG_NOSIGNAL );
		sent = gnutls_record_send( g->session, ptr, total );
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
			if ( sent == GNUTLS_E_INTERRUPTED || sent == GNUTLS_E_AGAIN ) {
				FPRINTF("TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( sent == EAGAIN || sent == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF( "Tried three times to write to socket. We're done.\n" );
					return 0;	
				}
				FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
				continue;
			}
			else {
				//this would just be some uncaught condition...
				FPRINTF( "Caught unknown condition: %s\n", gnutls_strerror( sent ) );
				return 0;
			}

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


#if 0
const int write_gnutls ( int fd, zhttp_t *rq, zhttp_t *rs, struct cdata *conn ) {
	FPRINTF( "Write started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->ctx->data;
	int sent = 0, pos = 0, try = 0, total = rs->mlen, pending;
	unsigned char *ptr = rs->msg;

#if 0
	int total = rs->mlen;
	int pos = 0;
	int try = 0;
#endif

	for ( ;; ) {
		sent = gnutls_record_send( g->session, ptr, total );
		FPRINTF( "Bytes sent: %d\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			FPRINTF( "total bytes remaining %d...\n", total );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			int pending = gnutls_record_check_pending( g->session ); 
			FPRINTF( "pending == %d, do we try again?\n", pending );
			if ( total == 0 ) {
				FPRINTF( "total == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;
			}
		}
	#if 1
		else {
			if ( sent != GNUTLS_E_INTERRUPTED || sent != GNUTLS_E_AGAIN || errno != EAGAIN || errno != EWOULDBLOCK ) {
				//This is some uncaught condition, probably one of these: 
				//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
				FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;
			}
		}
	#else
		else if ( sent == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
			//FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
		}
		else {
			//TODO: Can't close a most-likely closed socket.  What do you do?
			if ( sent == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
				0; //FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
			else {
				//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
				FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;	
			}
		}
	#endif
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}
#if 0
	for ( ;; ) {	
		int sent = gnutls_record_send( g->session, &rs->msg[ pos ], total );
		FPRINTF( "Wrote %d bytes at gnutls_record_send\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			//continue resending...
			pos += sent;
			total -= sent;
			int pending = gnutls_record_check_pending( g->session ); 
			FPRINTF( "pending == %d, do we try again?\n", pending );
			if ( total == 0 ) {
				FPRINTF( "total == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;	
			}
		}
		else {
			if ( sent == GNUTLS_E_INTERRUPTED || sent == GNUTLS_E_AGAIN ) {
				FPRINTF("TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( sent == EAGAIN || sent == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF( "Tried three times to write to socket. We're done.\n" );
					return 0;	
				}
				FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
				continue;
			}
			else {
				//this would just be some uncaught condition...
				FPRINTF( "Caught unknown condition: %s\n", gnutls_strerror( sent ) );
				return 0;
			}
		}
	}
#endif
	FPRINTF( "Write complete.\n" );
	return 1;
}
#endif
#endif

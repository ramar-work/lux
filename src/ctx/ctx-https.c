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


#define CKPATH "/home/ramar/wwws/"


#ifndef DISABLE_TLS

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





// Interval for fake polling here...
static const struct timespec __interval__ = { 0, 100000000 };



// Destroy the GnuTLS context per thread 
static void destroy_gnutls ( struct gnutls_abstr *g ) {
	gnutls_deinit( g->session );
	//gnutls_certificate_free_credentials( g->creds );
	free( g );
}




// Attempts to process all the certificates we've asked for.  If it can't find or process one, server just dies.
static int process_certs ( server_t *p, gnutls_certificate_credentials_t *t, int *count ) {
	int found = 0;

	// Find the certificates and keys that each host is pointing to 
	for ( struct lconfig **h = p->config->hosts; h && *h ; h++ ) {
		// Define everything here
		char crt[2048] = {0};
		char key[2048] = {0}; 
		char ca[2048] = {0};
		const char *fmt = "%s/%s/%s";
		const char *wwwroot = p->config->wwwroot;
		(*h)->tlsready = 0;
		found++;

		// Check that specified directory, TLS certifcate and key file exist
		if ( (*h)->dir ) {

			FPRINTF( "Attempting to process cert for host '%s'\n", (*h)->name );

			if ( (*h)->cert_file )
				snprintf( crt, sizeof(crt), fmt, wwwroot, (*h)->dir, (*h)->cert_file );
			else {
				snprintf( p->err, sizeof( p->err ), "Certificate file for host '%s' unspecified\n", (*h)->name );
				return 0;
			}
				
			if ( (*h)->key_file )
				snprintf( key, sizeof(key), fmt, wwwroot, (*h)->dir, (*h)->key_file );
			else {
				snprintf( p->err, sizeof( p->err ), "Key file for host '%s' unspecified\n", (*h)->name );
				return 0;
			}

			if ( access( crt, R_OK ) == -1 ) {
FPRINTF( "cert access was bad\n" );
				snprintf( p->err, sizeof( p->err ), "Certificate file %s for host '%s' inaccessible.\n", crt, (*h)->name );
				return 0;
			}

			if ( access( key, R_OK ) == -1 ) {
FPRINTF( "key access was bad\n" );
				snprintf( p->err, sizeof( p->err ), "Key file %s for host '%s' inaccessible.\n", key, (*h)->name );
				return 0;
			}

		#if 0
			if ( *ca && access( ca, R_OK ) == -1 ) {
				snprintf( p->err, sizeof( p->err ), "CA bundle file %s for host '%s' inaccessible\n", ca, (*h)->name );
				return 0;
			}

			// TODO: This should match the number of TLS enabled clients
			if ( *ca ) {
				int trust_set = 
					gnutls_certificate_set_x509_trust_file( g->x509_cred, ca, GNUTLS_X509_FMT_PEM );
				if ( trust_set < 0 ) {
					FPRINTF( "Could not set trust for '%s': %s\n", (*h)->name, gnutls_strerror( trust_set ) );
					return 0;
				}
				FPRINTF( "Certificates processed: %d\n", trust_set );
			}
		#endif

			// Simply load and associate
			int status = gnutls_certificate_set_x509_key_file( *t, crt, key, GNUTLS_X509_FMT_PEM );
			if ( status < 0 ) {
				snprintf( p->err, sizeof( p->err ), "Could not set tls info for '%s': %s\n", (*h)->name, gnutls_strerror( status ) );
				return 0;
			}

			(*h)->tlsready = 1;	
			(*count)++;
			FPRINTF( "TLS connection data for host '%s' successfully initialized\n", (*h)->name );
		}
	}

	if ( *count != found ) {
		snprintf( p->err, sizeof( p->err ), "No. of certs don't match with no. of sites served via TLS" );
		return 0;
	}

	return 1;
}



// Create a new GnuTLS context
int create_gnutls ( server_t *p ) {

	// Define
	int status = 0;
	int certcount = 0;
	const int size = sizeof( gnutls_certificate_credentials_t );
	gnutls_certificate_credentials_t *bob = NULL;


	// Initialize GnuTLS context
	if ( ( status = gnutls_global_init() ) != GNUTLS_E_SUCCESS ) {
		snprintf( p->err, sizeof( p->err ),
			"gnutls_global_init() failed - %s", gnutls_strerror( status ) );
		//FPRINTF( "%s\n", p->err );
		return 0;
	}


	// Certificate credentials allocate and set 
	if ( !( bob = malloc( size ) ) || !memset( bob, 0, size ) ) {
		snprintf( p->err, sizeof( p->err ),
			"allocation failure - %s", gnutls_strerror( status ) );
		//FPRINTF( "%s\n", p->err );
		return 0;
	}


	// Certificate credentials allocate and set 
	if ( ( status = gnutls_certificate_allocate_credentials( bob ) ) != GNUTLS_E_SUCCESS ) {
		snprintf( p->err, sizeof( p->err ),
			"certificate strucutre allocation failed - %s", gnutls_strerror( status ) );
		//FPRINTF( "%s\n", p->err );
		free( bob );
		gnutls_global_deinit();
		return 0;
	}


	// If we run it this way, it should just fail with no issue
	if ( !process_certs( p, bob, &certcount ) ) {
		gnutls_certificate_free_credentials( *bob );
		free( bob );
		gnutls_global_deinit();
		return 0;
	}


	// Set global context for this protocol
	p->data = bob;
	return 1;
}




// Functions to run before we can speak to others via TLS 
const int pre_gnutls ( server_t *p, conn_t *conn ) {

	// Define
	struct gnutls_abstr *g = NULL;
	int ret, invalid = 1, size = sizeof( struct gnutls_abstr );
	unsigned int snitype = GNUTLS_NAME_DNS;
	size_t snisize = CTXHTTPS_SNI_LENGTH;
	int certfound = 0;
	gnutls_certificate_credentials_t *cred =   
		(gnutls_certificate_credentials_t *)p->data;

	// Die if there are no credentials
	if ( !cred ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"No credentials available for GnuTLS" );
		return 0;
	}

	// Allocate a structure for the request process
	if ( !( g = malloc( size )) || !memset( g, 0, size ) ) { 
		snprintf( conn->err, sizeof( conn->err ),
			"per connection gnutls allocation failure - %s", strerror( errno ) );
		return 0;
	}

	// Start a GnuTLS session
	// TODO: You might need this too: GNUTLS_NO_SIGNAL 
	ret = gnutls_init( &g->session,
		GNUTLS_SERVER | GNUTLS_NONBLOCK | GNUTLS_NO_TICKETS );
	if ( ret != GNUTLS_E_SUCCESS ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"Failed to initialize new TLS session for incoming connection: %s\n", gnutls_strerror( ret ) );
		return 0;
	}


	// Set up default cipher suites, etc
	// TODO: Allow customization here in the future
	ret = gnutls_set_default_priority( g->session );
	if ( ret != GNUTLS_E_SUCCESS ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"Failed to set default priority for incoming connection: %s\n", gnutls_strerror( ret ) );
		return 0;
	}

	// NOTE: What is this doing?
	ret = gnutls_credentials_set( g->session, GNUTLS_CRD_CERTIFICATE, *cred );
	if ( ret != GNUTLS_E_SUCCESS ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"Failed to set credentials for incoming connection: %s\n", gnutls_strerror( ret ) );
		return 0;
	}


	//TODO: This might not be necessary
	//gnutls_certificate_server_set_request( g->session, GNUTLS_CERT_IGNORE ); 

	// Set a handshake timeout (perhaps a server or individual site option)
	gnutls_handshake_set_timeout( g->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	// Turn the open file into a secure socket
	gnutls_transport_set_int( g->session, conn->fd );

	// Perform and complete the handshake.  Get the server name as well.
	do {
		// TODO: Log any failures here
		ret = gnutls_handshake( g->session );
	#if 0
		// This is somewhat helpful debugging information
		FPRINTF( "HANDSHAKE STATUS: %s\n", gnutls_handshake_description_get_name( ret ) );
		FPRINTF( "CIPHERSUITE NAME: %s\n", gnutls_ciphersuite_get( g->session ) );
		char *sgd = gnutls_session_get_desc( g->session );
		FPRINTF( "CIPHERSUITE DESCRIPTION: %s\n", sgd );
		free( sgd );
	#endif
		ret = gnutls_handshake( g->session );

	#ifndef DISABLE_SNI
		// Get the server name
		if ( ret == GNUTLS_E_SUCCESS ) {
			int sret = gnutls_server_name_get( g->session, g->sniname, &snisize, &snitype, 0 );
			if ( sret < 0 || snisize == 0 ) {
				// TODO: Check if the client is even capable of this
				// ...
				snprintf( conn->err, sizeof( conn->err ), 
					"Could not get server name: %s\n", gnutls_strerror( sret ) );
				return 0;
			}
		
			// Check here that this is a valid host
			FPRINTF( "GOT SNI NAME: '%s'\n", g->sniname ); 
			for ( struct lconfig **h = conn->config->hosts; h && *h ; h++ ) {
				if ( (*h)->tlsready == 1 && ( !strcmp( g->sniname, (*h)->name ) || !strcmp( g->sniname, (*h)->alias ) ) ) {
					invalid = 0;
					break;
				}
			}

			// If we make it through the whole list and found no client, you need to exit
		}
		else if ( ret != GNUTLS_E_AGAIN && ret != GNUTLS_E_INTERRUPTED ) {
			// Handle what happens in these cases, maybe don't retry too many times
		}
	#endif
	}
	while ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );	

	if ( ret < 0 ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"GnuTLS handshake failed: %s\n", gnutls_strerror( ret ) );
		destroy_gnutls( g );
		return 0;
	}

	if ( invalid ) {
		snprintf( conn->err, sizeof( conn->err ), 
			"Invalid host '%s', requested\n", g->sniname );
		destroy_gnutls( g );
		return 0;	
	}


	FPRINTF( "GnuTLS handshake succeeded.\n" );
	//conn->ctx->data = g;
	conn->data = g;
	conn->stage = CONN_READ;
	return 1;
}



// Read a message that the server will use later.
const int read_gnutls ( server_t *p, conn_t *conn ) {

	//Set references, initialize pointers
	int total = 0, nsize, mult = 1, size = CTX_READ_SIZE; 
	int hlen = -1, mlen = 0, bsize = ZHTTP_PREAMBLE_SIZE;
	unsigned char *x = NULL, *xp = NULL;
	const unsigned short bhsize = 4;
	zhttp_t *rq = conn->req;
	zhttp_t *rs = conn->res;
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->data;

	//Get the time
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Bad certs can leave us with this sorry state
	if ( !g || !g->session ) {
		snprintf( conn->err, sizeof( conn->err ),
			"per connection gnutls allocation failure - %s", strerror( errno ) );
		return 0;
	}

	//Set another pointer for just the headers
	memset( x = rq->preamble, 0, ZHTTP_PREAMBLE_SIZE );

	//Read whatever the server sends and read until complete.
	for ( int rd, flags, recvd = -1; recvd < 0 || bsize <= 0;  ) {
		rd = gnutls_record_recv( g->session, x, bsize );
		if ( rd == 0 ) {
			//conn->count = -2; //most likely resources are unavailable
			//TODO: May need to tear down the connection.
			break;		
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
				FPRINTF( "TLS got handshake reauthentication request...\n" );
				continue;	
			}
			else {
				FPRINTF( "TLS got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
				snprintf( conn->err, sizeof( conn->err ), "%s",
					(char *)gnutls_strerror( rd ) );
				return 0;
			}

			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				snprintf( conn->err, sizeof( conn->err ), 
					"Got socket read error: %s\n", strerror( errno ) );
				return 0;
			}

			// Just stop trying to read if you get here
			if ( ( n.tv_sec - timer.tv_sec ) > p->timeout ) {
				conn->stage = CONN_WRITE;
				(void)http_set_error( rs, 408, "Timeout reached." );
				return 1;
			}

			//FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			FPRINTF( "Received %d additional header bytes on fd %d\n", rd, conn->fd ); 
			int pending = 0;
			bsize -= rd, total += rd, x += rd;
			recvd = http_header_received( rq->preamble, total ); 
			hlen = recvd; //rq->mlen = total;
			if ( recvd == ZHTTP_PREAMBLE_SIZE ) {
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

	//Stop if the header was just too big
	if ( hlen == -1 ) {
		conn->stage = CONN_WRITE;
		(void)http_set_error( rs, 500, "Header too large" ); 
		return 1;
	}

	//This should probably be a while loop
	if ( !http_parse_header( rq, hlen ) ) {
		conn->stage = CONN_WRITE;
		(void)http_set_error( rs, 500, (char *)rq->errmsg ); 
		return 1;
	}

	//If the message is not idempotent, stop and return.
	//print_httpbody( rq );
	if ( !rq->idempotent ) {
		//rq->atype = ZHTTP_MESSAGE_STATIC;
		conn->stage = CONN_PROC;
		FPRINTF( "Read complete, no content body (read %d bytes)\n", total );
		return 1;
	}

	//Check to see if we've fully received the message
	if ( total == ( hlen + bhsize + rq->clen ) ) {
		rq->msg = rq->preamble + ( hlen + bhsize );
		if ( !http_parse_content( rq, rq->msg, rq->clen ) ) {
			conn->stage = CONN_WRITE;
			(void)http_set_error( rs, 500, (char *)rq->errmsg ); 
			return 1;
		}
		FPRINTF( "Read complete, finished parsing content body (read %d bytes)\n", total );
		conn->stage = CONN_PROC;
		return 1;
	}

	//Check here if the thing is too big
	if ( rq->clen > CTX_READ_MAX ) {
		snprintf( conn->err, sizeof( conn->err ),
			"Content-Length (%d) exceeds read max (%d).", rq->clen, CTX_READ_MAX );
		conn->stage = CONN_WRITE;
		(void)http_set_error( rs, 500, (char *)conn->err ); 
		return 1;
	} 	

	#if 1
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
		snprintf( conn->err, sizeof( conn->err ),
			"Request queue full: %s.", strerror( errno ) ); 
		conn->stage = CONN_WRITE;
		(void)http_set_error( rs, 500, conn->err );
		return 1;
	}

	//Take any excess in the preamble and move that into xp
	int crecvd = total - ( hlen + bhsize );
	//FPRINTF( "crecvd: %d, %d, %d, %d\n", crecvd, ( hlen + bhsize ), nsize, rq->clen );
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
		
		// TODO: Are there any cases in which this would literally be zero?
		if ( ( rd = gnutls_record_recv( g->session, xp, bsize ) ) == 0 ) {
			//
			conn->stage = CONN_PROC;
			return 1;
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
				//FPRINTF( "TLS got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
				snprintf( conn->err, sizeof( conn->err ), "%s",
					(char *)gnutls_strerror( rd ) );
				return 0;
			}
			
			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				//TODO: This should be logged somewhere
				snprintf( conn->err, sizeof( conn->err ),
					"Got socket read error: %s\n", strerror( errno ) );
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) >= p->timeout ) {
				conn->count = -3;
				conn->stage = CONN_WRITE;
				(void)http_set_error( rs, 408, "Timeout reached." );
				return 1;
			}

			FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			//Process a successfully read buffer
			FPRINTF( "Received %d additional bytes on fd %d\n", rd, conn->fd ); 
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
		conn->stage = CONN_WRITE;
		return http_set_error( rs, 500, (char *)rq->errmsg ); 
	}

	FPRINTF( "Read complete (read %d out of %d bytes for content)\n", crecvd, rq->clen );
	conn->stage = CONN_PROC;
	return 1;
}



// Write a message to secure socket
const int write_gnutls ( server_t *p, conn_t *conn ) {
	FPRINTF( "Write started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->data;
	int sent = 0, pos = 0, pending;
	zhttp_t *rq = conn->req;
	zhttp_t *rs = conn->res;
	unsigned char *ptr = rs->msg;
	int total = rs->mlen;

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );

	// For now, we're not rewriting anything or starting again.
	conn->stage = CONN_POST;

	// TODO: Still need to be mindful of who can and cannot support sendfile
     #ifdef SENDFILE_ENABLED
	if ( rs->atype == ZHTTP_MESSAGE_SENDFILE ) {

FPRINTF( "header length is %d\n", total );
		//Send the header first
		int hlen = total;	
		for ( ; total; ) {
			sent = gnutls_record_send( g->session, ptr, total );
			if ( sent == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes of header have been sent...\n", rs->mlen );
				break;
			}
			else if ( sent > -1 ) {
				pos += sent, total -= sent, ptr += sent;	
				FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
				int pending = gnutls_record_check_pending( g->session ); 
				FPRINTF( "pending == %d, do we try again?\n", pending );
				if ( total == 0 ) {
					FPRINTF( "sent == 0, assuming all %d bytes of header have been sent...\n", rs->mlen );
					break;
				}
			}
			else {
				if ( sent != GNUTLS_E_INTERRUPTED && sent != GNUTLS_E_AGAIN ) { 
					//This is some uncaught condition, probably one of these: 
					//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
					snprintf( conn->err, sizeof( conn->err ), 
						"Got socket write error: %s\n", gnutls_strerror( errno ) );
					return 0;
				}

				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					//This is some uncaught condition, probably one of these: 
					//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
					conn->stage = CONN_POST;
					snprintf( conn->err, sizeof( conn->err ), 
						"Got socket write error: %s\n", strerror( errno ) );
					return 0;
				}

				if ( !total ) {
					FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
					return 1;
				}
				
				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) >= p->timeout ) {
					// Cut if we can't get this message out for some reason
					snprintf( conn->err, sizeof( conn->err ), 
						"Timeout reached on write end of socket - header." );
					return 0;
				}

				FPRINTF("Trying again to send header to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}
		FPRINTF( "Header write complete (sent %d out of %d bytes)\n", pos, hlen );

		//Then send the file
FPRINTF( "content-length is %d\n", rs->clen );
		for ( total = rs->clen; total; ) {
			//sent = sendfile( fd, rs->fd, NULL, CTX_WRITE_SIZE );
			//TODO: Test this extensively, g->session should hold an open file
			sent = gnutls_record_send_file( g->session, rs->fd, NULL, CTX_WRITE_SIZE );
			FPRINTF( "Bytes sent from open file %d: %d\n", rs->fd, sent );
			if ( sent == 0 )
				break;
			else if ( sent > -1 )
				total -= sent, pos += sent;
			else {
				if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
					snprintf( conn->err, sizeof( conn->err ), 
						"Got socket write error: %s\n", strerror( errno ) );
					return 0;	
				}

				struct timespec n = {0};
				clock_gettime( CLOCK_REALTIME, &n );
				
				if ( ( n.tv_sec - timer.tv_sec ) > p->timeout ) {
					snprintf( conn->err, sizeof( conn->err ), 
						"Timeout reached on write end of socket - body." );
					return 0;
				}

				FPRINTF("Trying again to send file to socket. (%d).\n", sent );
				nanosleep( &__interval__, NULL );
			}
			FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
		}

		FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, rs->clen );
		return 1;
	}
     #endif


	// Start writing data to socket
	for ( ;; ) {
		sent = gnutls_record_send( g->session, ptr, total );
		FPRINTF( "Bytes sent: %d, over file %d\n", sent, conn->fd );
		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			if ( total == 0 ) {
				FPRINTF( "total == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;
			}
		}
		else {
			FPRINTF( "Caught error condition: %d, %s\n", sent, gnutls_strerror( sent ) );
			if ( sent == GNUTLS_E_INTERRUPTED || sent == GNUTLS_E_AGAIN ) {
				FPRINTF("TLS was interrupted...  Try request again...\n" );
				continue;	
			}
			#if 0
			else if ( sent == EAGAIN || sent == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF( "Tried three times to write to socket. We're done.\n" );
					return 0;	
				}
				FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
				continue;
			}
			#endif
			else {
				//this would just be some uncaught condition...
				snprintf( conn->err, sizeof( conn->err ), 
					"Caught unknown condition: %s\n", gnutls_strerror( sent ) );
				return 0;
			}

			if ( !total ) {
				FPRINTF( "sent == %d, total = 0:  Message should be done\n", sent );
				return 1;
			}
			
			if ( errno != EAGAIN || errno != EWOULDBLOCK ) {
				snprintf( conn->err, sizeof( conn->err ), 
					"Got socket write error: %s\n", strerror( errno ) );
				return 0;	
			}

			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );
			
			if ( ( n.tv_sec - timer.tv_sec ) > p->timeout ) {
				snprintf( conn->err, sizeof( conn->err ), 
					"Timeout reached on write end of socket - body." );
				return 0;
			}

			nanosleep( &__interval__, NULL );
		}
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}

	// Should I explicitly close?  Let's try
	FPRINTF( "Write complete (sent %d out of %d bytes)\n", pos, rq->mlen );
	return 1;
}



// End GnuTLS 
const int post_gnutls ( server_t *p, conn_t *conn ) {
	FPRINTF( "Shutting down TLS connection and closing write end\n" );
	// Close TLS sesssion
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->data;
	int stat = 0;
	if ( ( stat = gnutls_bye( g->session, GNUTLS_SHUT_RDWR ) ) < 0 ) {
		FPRINTF( "Could not shut down GnuTLS connection: %s.\n", gnutls_strerror( stat ) );
	}

	// Destroy the context and the session
	destroy_gnutls( g );

	FPRINTF( "Successfully shut down TLS connection.\n" );
	return 1;
}



// Destroy the GnuTLS context completely
void free_gnutls( server_t *p ) {
	FPRINTF( "Destroying pre data for current protocol.\n" );
	gnutls_certificate_credentials_t *cred =   
		(gnutls_certificate_credentials_t *)p->data;
	gnutls_certificate_free_credentials( *cred );
	free( p->data );
	p->data = NULL;
	gnutls_global_deinit();
	FPRINTF( "GnuTLS deinit should have run.\n" );
}




#if 0
	//make a buffer
	int randfd = 0, writefd = 0;
	unsigned char fn[16] = {0};
	randfd = open( "/dev/urandom", O_RDONLY );
 	memset( fn, 0, sizeof( fn ) );	
	read( fd, fn, 15 );
	close( randfd );

	//filename
	char filename[128]={0};
	snprintf( filename, 127, "/tmp/%04d-hyppie", fd );
FPRINTF( "fn: %s\n", filename );
	writefd = open( filename, O_RDWR | O_CREAT );
	
	// boy, this is a bit gnarly	
	char buff[ 128 ] = {0};
	snprintf( buff, 127, "d1: %p, d2: %p\n", gcc, conn->ctx->data );
	write( writefd, buff, strlen( buff ) );
	close( writefd );
FPRINTF( "EXITING THREAD\n" );
	pthread_exit(0);
#endif

#endif


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


const int post_gnutls ( int fd, struct HTTPBody *a, struct HTTPBody *b, struct cdata *conn) {
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
		FPRINTF( "Host %s will init SSL: %s\n", (*h)->name, ( (*h)->dir && (*h)->cert_file ) ? "Y" : "N" );
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
pre_gnutls ( int fd, struct HTTPBody *a, struct HTTPBody *b, struct cdata *conn ) {
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



const int 
read_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Read started...\n" );

	//Define
	struct gnutls_abstr *g = (struct gnutls_abstr *)conn->ctx->data;
	int mult = 1, size = CTX_READ_SIZE;
	char err[ 2048 ] = {0};

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Allocate space for the first call
	if ( !( rq->msg = malloc( size ) ) ) {
		return http_set_error( rs, 500, "Could not allocate initial read buffer." ); 
	}

	//Bad certs can leave us with this sorry state
	if ( !g || !g->session ) {
		//set conn->count to stop here...
		return http_set_error( rs, 500, "SSL/TLS handshake error encountered." ); 
	}
 
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

			//Handle any SSL/TLS errors
			if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				FPRINTF( "SSL was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				FPRINTF( "SSL got handshake reauth request...\n" );
				continue;	
			}
			else {
				FPRINTF( "SSL got error code: %d = %s.\n", rd, gnutls_strerror( rd ) );
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
			struct HTTPBody *tmp = http_parse_request( rq, err, sizeof(err) ); 
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


const int 
write_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
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
				FPRINTF("SSL was interrupted...  Try request again...\n" );
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

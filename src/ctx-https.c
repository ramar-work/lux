/* ------------------------------------------- * 
 * ctx-https.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTPS contexts.
 *
 * Usage
 * -----
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "ctx-https.h"

#define CHECK(x) assert((x)>=0)

void create_gnutls( void **p ) {
	CHECK( gnutls_global_init() );
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


const int post_gnutls ( int fd, struct cdata *conn, void **p ) {
#if 0
	struct gnutls_abstr *g = (struct gnutls_abstr *)*p;
	destroy_gnutls( g );
#endif
	return 0;
}


static int process_credentials ( struct gnutls_abstr *g, struct host **hosts ) {
	while ( *hosts ) {
		char *dir = (*hosts)->dir;
		//each of the files need to be loaded somehow...
		if ( dir && (*hosts)->certfile  ) {
			//Make a filename
			char cert[2048] = {0}, key[2048] = {0}, ca[2048] = {0};
			snprintf( cert, sizeof(cert), "%s/%s", dir, (*hosts)->certfile );
			snprintf( key, sizeof(key), "%s/%s", dir, (*hosts)->keyfile );
			snprintf( ca, sizeof(ca), "%s/%s", dir, (*hosts)->ca_bundle );
			#if 0
			FPRINTF( "Attempting to process these certs\n" );
			FPRINTF( "ca bundle: %s\n", ca );
			FPRINTF( "cert: %s\n", cert );
			FPRINTF( "key: %s\n", key );
			#endif

			//Will this ever be negative?	
			int cp = gnutls_certificate_set_x509_trust_file( g->x509_cred, ca, GNUTLS_X509_FMT_PEM );
			if ( cp < 0 ) {
				FPRINTF( "Could not set trust for '%s': %s\n", (*hosts)->name, gnutls_strerror( cp ) );
				return 0;
			}
			FPRINTF( "Certificates processed: %d\n", cp );
			int status = gnutls_certificate_set_x509_key_file( g->x509_cred, cert, key, GNUTLS_X509_FMT_PEM );
			if ( status < 0 ) {
				FPRINTF( "Could not set certificate for '%s': %s\n", (*hosts)->name, gnutls_strerror( status ) );
				return 0;
			}
		}
		hosts++;
	}	
	return 1;
}


const int pre_gnutls ( int fd, struct cdata *conn , void **p ) {

#if 0
	//Allocate a structure for the request process
	struct gnutls_abstr *g;
	int ret, size = sizeof( struct gnutls_abstr );
	if ( !( g = malloc( size ))  || !memset( g, 0, size ) ) { 
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
	//struct host **hosts = config->hosts;
	if ( !process_credentials( g, config->hosts ) ) {
		gnutls_certificate_free_credentials( g->x509_cred );
		free( g );
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

	//Do the actual handshake with an open file descriptor
	do {
		ret = gnutls_handshake( g->session );
	}
	while ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );	

	if ( ret < 0 ) {
		destroy_gnutls( g );
		FPRINTF( "GnuTLS handshake failed: %s\n", gnutls_strerror( ret ) );
		//This isn't a fatal error... but what do I return?
		return 0;	
	}

	FPRINTF( "GnuTLS handshake succeeded.\n" );
	*p = g;
#endif
	return 1;
}


const int read_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *p ) {
	FPRINTF( "Read started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)p->ctx->data;
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	
	char err[ 2048 ] = {0};

	//Read first
	for ( ;; ) {	
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );
		FPRINTF( "Attempting to read from TLS socket?\n" );
		int rd = gnutls_record_recv( g->session, buf2, sizeof( buf2 ));
		FPRINTF( "Read %d bytes at gnutls_record_recv\n", rd );

		//Read into a static buffer
		if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			FPRINTF( "Message should be fully read.\n" );
			rq->msg = buf;
			break;
		}
		else if ( rd > 0 ) {
			FPRINTF( "Message read incomplete.\n" );
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...
			write( 2, rq->msg, rq->mlen );
			int pending = gnutls_record_check_pending( g->session ); 
			if ( pending == 0 ) {
				FPRINTF( "No data left in buffer?\n" );
				break;
			}
			else {
				FPRINTF( "Data left in buffer: %d bytes\n", pending );
				continue;
			}
		}
		else { //if ( rd < 0 ) {
			FPRINTF( "Couldn't read all of message...\n" );
			if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				FPRINTF("SSL was interrupted...  Try request again...\n" );
				continue;	
			}
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				FPRINTF("SSL got handshake reauth request...\n" );
				continue;	
			}
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF("Tried three times to read from socket. We're done.\n" );
					//FPRINTF("rq->mlen: %d\n", rq->mlen );
					//FPRINTF("%p\n", buf );
					//rq->msg = buf;
					break;
				}
				FPRINTF("Tried %d times to read from socket. Trying again?.\n", try );
				continue;	
			}
			else {
				FPRINTF("SSL got error code: %d, meaning '%s'.\n", rd, gnutls_strerror( rd ) );
				//this would just be some uncaught condition...
				return http_set_error( rs, 500, (char *)gnutls_strerror( rd ) );
			}
		}
	}

	if ( !http_parse_request( rq, err, sizeof(err) ) ) {
		return http_set_error( rs, 500, err ); 
	}
	FPRINTF( "Read complete.\n" );
	return 1;
}


const int write_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *p ) {
	FPRINTF( "Write started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)p->ctx->data;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

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
	FPRINTF( "Write complete.\n" );
	return 1;
}




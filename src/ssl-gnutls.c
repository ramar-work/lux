#include "ssl-gnutls.h"

#define CHECK(x) assert((x)>=0)

void * create_gnutls() {
	struct gnutls_abstr *g = malloc( sizeof(struct gnutls_abstr) );
	if ( !g || !memset( g, 0, sizeof( struct gnutls_abstr ) ) ) {
		FPRINTF( "Failed to allocate space for gnutls_abstr\n" );
		return NULL;
	}
	
	g->x509_cred = NULL;
	g->cafile = strdup( CAFILE ); 
	//g->crlfile = strdup( CRLFILE ); 
	g->certfile = strdup( CERTFILE );
	g->keyfile = strdup( KEYFILE ); 
	g->priority_cache = NULL;
	//Log TLS errors with this...
	//gnutls_global_set_log_function( tls_log_func );
	CHECK( gnutls_global_init() );
#if 0
	CHECK( gnutls_certificate_allocate_credentials( &g->x509_cred) );
	//find the certificate authority to use
	CHECK( gnutls_certificate_set_x509_trust_file( g->x509_cred, g->cafile, GNUTLS_X509_FMT_PEM ) );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, g->crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	CHECK( gnutls_certificate_set_x509_key_file( g->x509_cred, g->certfile, g->keyfile, GNUTLS_X509_FMT_PEM ) );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	CHECK( gnutls_priority_init( &g->priority_cache, NULL, NULL ) );
#endif
	return g;
}


int accept_gnutls ( struct sockAbstr *su, int *child, char *err, int errlen ) {
	//struct gnutls_abstr *g = (struct gnutls_abstr *)su->ssl_ctx->userdata;
	struct rwctx *v = su->ctx;
	struct gnutls_abstr *g = (struct gnutls_abstr *)v->userdata;
	 

	CHECK( gnutls_certificate_allocate_credentials( &g->x509_cred) );
	//find the certificate authority to use
	CHECK( gnutls_certificate_set_x509_trust_file( g->x509_cred, g->cafile, GNUTLS_X509_FMT_PEM ) );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, g->crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	CHECK( gnutls_certificate_set_x509_key_file( g->x509_cred, g->certfile, g->keyfile, GNUTLS_X509_FMT_PEM ) );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	CHECK( gnutls_priority_init( &g->priority_cache, NULL, NULL ) );

	gnutls_init( &g->session, GNUTLS_SERVER );
	gnutls_priority_set( g->session, g->priority_cache );
	gnutls_credentials_set( g->session, GNUTLS_CRD_CERTIFICATE, g->x509_cred );
	gnutls_certificate_server_set_request( g->session, GNUTLS_CERT_IGNORE ); 
	gnutls_handshake_set_timeout( g->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 

	//Seems like this could be very slow...
	*child = accept( su->fd, &su->addrinfo, &su->addrlen ); 

	//Handshake?
	int ret;
  gnutls_transport_set_int( g->session, *child );
	do {
		ret = gnutls_handshake( g->session );
	}
	while ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );	

	if ( ret < 0 ) {
		close( *child );
		*child = 0;
		gnutls_deinit( g->session );
		FPRINTF( "GnuTLS handshake failed: %s\n", gnutls_strerror( ret ) );
		return 0;
	}

	FPRINTF( "GnuTLS handshake succeeded.\n" );
	return 1;
}


int read_gnutls( int fd, void *ctx, uint8_t *buffer, int size ) {
	struct gnutls_abstr *g = (struct gnutls_abstr *)ctx;
#if 1
	int rd = 0;
	char buf[ 2048 ];
	int sz = sizeof(buf) - 1;
	memset( buf, 0, sizeof( buf ) );

	//gnutls_session_t *ss = (gnutls_session_t *)sess;
	if ( ( rd = gnutls_record_recv( g->session, buf, sz ) ) > 0 ) {
		FPRINTF( "SSL might be fine... got %d from gnutls_record_recv\n", rd );
	}
	else if ( rd == GNUTLS_E_REHANDSHAKE ) {
		FPRINTF("SSL got handshake reauth request..." );
		//TODO: There should be a seperate function that handles this.
		//It's a fail for now...	
		return 0;
	}
	else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
		FPRINTF("SSL was interrupted...  Try request again...\n" );
		//continue;
	}
	else {
		FPRINTF("SSL got error code: %d, meaning '%s'.\n", rd, gnutls_strerror( rd ) );
		//continue;
	}
#endif
	return 1;
}


int write_gnutls( int fd, void *ctx, uint8_t *buffer, int size ) {
	return 1;
}


void destroy_gnutls( void *ctx ) {
}

#if 0
int read_gnutls( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );
#if 1
	//Read all the data from a socket.
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	
	char err[ 2048 ] = {0};

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//Read into a static buffer
		if ( ( rd = recv( fd, buf2, size, MSG_DONTWAIT ) ) == --1 ) {
			//A subsequent call will tell us a lot...
			FPRINTF( "Couldn't read all of message...\n" );
			//whatsockerr( errno );
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
					FPRINTF("Tried three times to read from socket. We're done.\n" );
					FPRINTF("rq->mlen: %d\n", rq->mlen );
					FPRINTF("%p\n", buf );
					//rq->msg = buf;
					break;
				}
				FPRINTF("Tried %d times to read from socket. Trying again?.\n", try );
			}
			else {
				//this would just be some uncaught condition...
				return http_set_error( rs, 500, strerror( errno ) );
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			rq->msg = buf;
			break;
		}
	#if 0
		else if ( rd == GNUTLS_E_REHANDSHAKE ) {
			fprintf(stderr, "SSL got handshake reauth request..." );
			continue;
		}
		else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
			fprintf(stderr, "SSL was interrupted...  Try request again...\n" );
			continue;
		}
	#endif
		else {
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
#if 0
			fprintf(stderr, "buf: %p\n", buf );
			fprintf(stderr, "buf2: %p\n", buf2 );
			fprintf(stderr, "pos: %d\n", bfsize - size );
#endif
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			FPRINTF( "Recvd %d bytes on fd %d\n", rd, fd ); 
		}
	}

	if ( !http_parse_request( rq, err, sizeof(err) ) ) {
		return http_set_error( rs, 500, err ); 
	}
#endif
	FPRINTF( "Read complete.\n" );
	return 1;
}
#endif

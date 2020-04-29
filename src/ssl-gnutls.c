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
	struct sslctx *v = su->ssl_ctx;
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
exit(0);
	return 1;
}




int handshake_gnutls( void *ctx, int fd ) {
	struct gnutls_abstr *g = (struct gnutls_abstr *)ctx;
#if 1
	int success=0;
	//SSL again
	gnutls_session_t session, *sptr = NULL;
	if ( 1 ) {
		gnutls_init( &g->session, GNUTLS_SERVER );
		gnutls_priority_set( g->session, g->priority_cache );
		gnutls_credentials_set( g->session, GNUTLS_CRD_CERTIFICATE, g->x509_cred );
		//NOTE: I need to do this b/c clients aren't expected to send a certificate with their request
		gnutls_certificate_server_set_request( g->session, GNUTLS_CERT_IGNORE ); 
		gnutls_handshake_set_timeout( g->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
		//Bind the current file descriptor to GnuTLS instance.
		gnutls_transport_set_int( g->session, fd );
		//Do the handshake here
		//TODO: I write nothing that looks like this, please refactor it...
		int success = 0;
		do {
			success = gnutls_handshake( g->session );
		} while ( success == GNUTLS_E_AGAIN || success == GNUTLS_E_INTERRUPTED );	
		if ( success < 0 ) {
			close( fd );
			gnutls_deinit( g->session );
			//TODO: Log all handshake failures.  Still don't know where.
			FPRINTF( "SSL handshake failed.\n" );
			//continue;
		}
		FPRINTF( "SSL handshake successful.\n" );
		sptr = &g->session;
	}
#endif
	return 1;
}

int read_gnutls( void *ctx ) {
	struct gnutls_abstr *g = (struct gnutls_abstr *)ctx;
#if 1
	int rd = 0;
	char buf[ 2048 ];
	int size = sizeof(buf) - 1;
	memset( buf, 0, sizeof( buf ) );

	//gnutls_session_t *ss = (gnutls_session_t *)sess;
	if ( ( rd = gnutls_record_recv( g->session, buf, size ) ) > 0 ) {
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

int write_gnutls( void *ctx ) {
	return 1;
}

void destroy_gnutls( void *ctx ) {
}


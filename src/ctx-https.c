#include "ctx-https.h"

#define CHECK(x) assert((x)>=0)

void create_gnutls( void **p ) {
	struct gnutls_abstr *g;

	if ( !( g = malloc( sizeof(struct gnutls_abstr)) ) ) { 
		FPRINTF( "Failed to allocate space for gnutls_abstr\n" );
		return;
	}

	memset( g, 0, sizeof( struct gnutls_abstr ) );
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
	FPRINTF( "gnutls struct: %p\n", g );
	*p = g;
	FPRINTF( "gnutls struct: %p\n", *p );
}


int accept_gnutls ( struct sockAbstr *su, int *child, void *p, char *err, int errlen ) {
	struct gnutls_abstr *g = (struct gnutls_abstr *)p;
	int ret;
	//struct rwctx *v = su->ctx;
	//struct gnutls_abstr *g = (struct gnutls_abstr *)p->userdata;
	
	//set up gnutls
	CHECK( gnutls_certificate_allocate_credentials( &g->x509_cred) );
	//find the certificate authority to use
	CHECK( gnutls_certificate_set_x509_trust_file( g->x509_cred, g->cafile, GNUTLS_X509_FMT_PEM ) );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, g->crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	CHECK( gnutls_certificate_set_x509_key_file( g->x509_cred, g->certfile, g->keyfile, GNUTLS_X509_FMT_PEM ) );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	CHECK( gnutls_priority_init( &g->priority_cache, NULL, NULL ) );

	gnutls_init( &g->session, GNUTLS_SERVER | GNUTLS_NONBLOCK );
	gnutls_priority_set( g->session, g->priority_cache );
	gnutls_credentials_set( g->session, GNUTLS_CRD_CERTIFICATE, g->x509_cred );
	gnutls_certificate_server_set_request( g->session, GNUTLS_CERT_IGNORE ); 
	gnutls_handshake_set_timeout( g->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 

	//Seems like this could be very slow...
	if (( *child = accept( su->fd, &su->addrinfo, &su->addrlen )) == -1 ) {
		//TODO: Need to check if the socket was non-blocking or not...
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			//This should just try to read again
			FPRINTF( "Try accept again.\n" );
			return AC_EAGAIN;	
		}
		else if ( errno == EMFILE || errno == ENFILE ) { 
			//These both refer to open file limits
			FPRINTF( "Too many open files, try closing some requests.\n" );
			return AC_EMFILE;	
		}
		else if ( errno == EINTR ) { 
			//In this situation we'll handle signals
			FPRINTF( "Signal received. (Not coded yet.)\n" );
			return AC_EEINTR;	
		}
		else {
			//All other codes really should just stop. 
			snprintf( err, errlen, "accept() failed: %s\n", strerror( errno ) );
			return 0;
		}
	}

	//Handshake?
  gnutls_transport_set_int( g->session, *child );
	do {
		ret = gnutls_handshake( g->session );
	}
	while ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED );	

	if ( ret < 0 ) {
		//close( *child );
		//*child = 0;
		gnutls_deinit( g->session );
		FPRINTF( "GnuTLS handshake failed: %s\n", gnutls_strerror( ret ) );
		//This isn't a fatal error...
		return AC_EAGAIN;	
	}

	FPRINTF( "GnuTLS handshake succeeded.\n" );
	return 1;
}


int read_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)p;
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


int write_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Write started...\n" );
	struct gnutls_abstr *g = (struct gnutls_abstr *)p;
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


void destroy_gnutls( void *ctx ) {
}


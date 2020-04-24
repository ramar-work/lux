#include "ssl.h"

//All of the cert stuff needs to be thrown in...
void open_ssl_context ( struct gnutls_abstr *g ) {
	gnutls_certificate_credentials_t x509_cred = NULL;
  gnutls_priority_t priority_cache;
	const char *cafile, *crlfile, *certfile, *keyfile;
	#if 0
	cafile = 
	crlfile = 
	#endif
	#if 0
	//These should always be loaded, and there will almost always be a series
	certfile = 
	keyfile = 
	#else
#define MPATH "/home/ramar/prj/hypno/certs/collinsdesign.net"
	//Hardcode these for now.
	certfile = MPATH "/collinsdesign_net.crt";
	keyfile = MPATH "/server.key";
	#endif
	//Obviously, this is great for debugging TLS errors...
	//gnutls_global_set_log_function( tls_log_func );
	gnutls_global_init();
	gnutls_certificate_allocate_credentials( &x509_cred );
	//find the certificate authority to use
	//gnutls_certificate_set_x509_trust_file( x509_cred, cafile, GNUTLS_X509_FMT_PEM );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	gnutls_certificate_set_x509_key_file( x509_cred, certfile, keyfile, GNUTLS_X509_FMT_PEM );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	gnutls_priority_init( &priority_cache, NULL, NULL );
}



void fill_session( struct gnutls_abstr *g, int fd ) {
#if 0
int success=0;
			//SSL again
			gnutls_session_t session, *sptr = NULL;
			if ( 1 ) {
				gnutls_init( &session, GNUTLS_SERVER );
				gnutls_priority_set( session, priority_cache );
				gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, x509_cred );
				//NOTE: I need to do this b/c clients aren't expected to send a certificate with their request
				gnutls_certificate_server_set_request( session, GNUTLS_CERT_IGNORE ); 
				gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
				//Bind the current file descriptor to GnuTLS instance.
				gnutls_transport_set_int( session, fd );
				//Do the handshake here
				//TODO: I write nothing that looks like this, please refactor it...
				int success = 0;
				do {
					success = gnutls_handshake( session );
				} while ( success == GNUTLS_E_AGAIN || success == GNUTLS_E_INTERRUPTED );	
				if ( success < 0 ) {
					close( fd );
					gnutls_deinit( session );
					//TODO: Log all handshake failures.  Still don't know where.
					fprintf( stderr, "%s\n", "SSL handshake failed." );
					continue;
				}
				fprintf( stderr, "%s\n", "SSL handshake successful." );
				sptr = &session;
			}
#endif
}


#if 0
//I'm reluctant to throw this out...
	gnutls_certificate_credentials_t x509_cred = NULL;
  gnutls_priority_t priority_cache;
	const char *cafile, *crlfile, *certfile, *keyfile;
	#if 0
	cafile = 
	crlfile = 
	#endif
	#if 0
	//These should always be loaded, and there will almost always be a series
	certfile = 
	keyfile = 
	#else
#define MPATH "/home/ramar/prj/hypno/certs/collinsdesign.net"
	//Hardcode these for now.
	certfile = MPATH "/collinsdesign_net.crt";
	keyfile = MPATH "/server.key";
	#endif
	//Obviously, this is great for debugging TLS errors...
	//gnutls_global_set_log_function( tls_log_func );
	gnutls_global_init();
	gnutls_certificate_allocate_credentials( &x509_cred );
	//find the certificate authority to use
	//gnutls_certificate_set_x509_trust_file( x509_cred, cafile, GNUTLS_X509_FMT_PEM );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	gnutls_certificate_set_x509_key_file( x509_cred, certfile, keyfile, GNUTLS_X509_FMT_PEM );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	gnutls_priority_init( &priority_cache, NULL, NULL );
#endif
			#if 0
			//SSL again
			gnutls_session_t session, *sptr = NULL;
			if ( values.ssl ) {
				gnutls_init( &session, GNUTLS_SERVER );
				gnutls_priority_set( session, priority_cache );
				gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, x509_cred );
				//NOTE: I need to do this b/c clients aren't expected to send a certificate with their request
				gnutls_certificate_server_set_request( session, GNUTLS_CERT_IGNORE ); 
				gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
				//Bind the current file descriptor to GnuTLS instance.
				gnutls_transport_set_int( session, fd );
				//Do the handshake here
				//TODO: I write nothing that looks like this, please refactor it...
				int success = 0;
				do {
					success = gnutls_handshake( session );
				} while ( success == GNUTLS_E_AGAIN || success == GNUTLS_E_INTERRUPTED );	
				if ( success < 0 ) {
					close( fd );
					gnutls_deinit( session );
					//TODO: Log all handshake failures.  Still don't know where.
					fprintf( stderr, "%s\n", "SSL handshake failed." );
					continue;
				}
				fprintf( stderr, "%s\n", "SSL handshake successful." );
				sptr = &session;
			}
			#endif

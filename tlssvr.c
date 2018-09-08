#include "vendor/single.h"
#include <gnutls/gnutls.h>
#define PROG "gx"
#define KEYFILE "key.pem"
#define CERTFILE "cert.pem"
#define CAFILE "/etc/ssl/certs/ca-certificates.crt"
#define CRLFILE "crl.pem"
#define OCSP_STATUS_FILE "crl.pem"
//#define errexit(c, m) do { fprintf( stderr, PROG ": %s\n", m); return c; } while(0)
#define errexit(c, ...) do { fprintf( stderr, "%s: ", PROG ); fprintf( stderr, __VA_ARGS__); return c; } while(0)
#define errdie(c) if ( c < 0 ) { fprintf( stderr, PROG ": %s\n", gnutls_strerror( c )); return c; }

int main (int argc, char *argv[]) {

	int err, ret, sd, ii, type, len;
	unsigned int status;
	char buf[ 2048 ] = { 0 }, *desc = NULL;
	Socket s = { .server   = 0, .proto    = "tcp" };
	gnutls_session_t session;
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
	char msg[ 32000 ] = { 0 };
	char GetMsg[2048] = { 0 };	
	//	"Content-Type: text/html\r\n"
	char GetMsgFmt[] = 
		"GET / HTTP/1.1\r\n"
		"Host: %s\r\n\r\n"
	;

	if ( !gnutls_check_version("3.4.6") ) { 
		errexit( 0, "GnuTLS 3.4.6 or later is required for this example." );	
	}

	//Is this needed?
	gnutls_global_init();
	if (( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0)
		errexit( err, gnutls_strerror( err ));

	if ((err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )
		errexit( err, gnutls_strerror( err ));
	/*
	//Set client certs this way...
	gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
	*/	

	//Initialize the session
	char *hostname = NULL;
	//hostname = "ramarcollins.com";
	hostname = "www.deep909.com";


	//Initialize gnutls and set things up
	if (( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 )
		errexit( err, gnutls_strerror( err ));

	if (( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, hostname, strlen(hostname)) ) < 0)
		errexit( err, gnutls_strerror( err ));

	if (( err=gnutls_set_default_priority( session ) ) < 0)
		errexit( err, gnutls_strerror( err ));
	
	if (( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0)
		errexit( err, gnutls_strerror( err ));

	gnutls_session_set_verify_cert( session, hostname, 0 );

	//Do socket connect (but after initial connect, I need the file desc)
	if ( !socket_connect( &s, hostname, 443 ) )
		errexit (0, "Couldn't connect to site... " );

	//?
	//fprintf( stderr, "s.fd: %d\n", s.fd );
	gnutls_transport_set_int( session, s.fd );
	gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	//This is ass ugly...
	do {
		ret = gnutls_handshake( session );
	} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

	if ( ret < 0 ) {
		fprintf( stderr, "ret: %d\n", ret );
		if ( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) {	
			type = gnutls_certificate_type_get( session );
			status = gnutls_session_get_verify_cert_status( session );
			err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
			fprintf( stdout, "cert verify output: %s\n", out.data );
			gnutls_free( out.data );
			//jump to end, but I don't do go to
		}
		errexit( 0, "Handshake failed: %s\n", gnutls_strerror( ret ));
	}
	else {
		desc = gnutls_session_get_desc( session );
		fprintf( stdout, "- Session info: %s\n", desc );
		gnutls_free( desc );
	}


	//pack a message
	len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, hostname );
 	
#if 1
	if (( err = gnutls_record_send( session, GetMsg, len ) ) < 0 )
		errexit( 0, "GnuTLS 3.4.6 or later is required for this example." );	

	ret = gnutls_record_recv( session, buf, sizeof(buf));
	if ( ret == 0 ) {
		fprintf( stderr, " - Peer has closed the TLS Connection\n" );
		goto end;
	}
	else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
		fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
	}
	else if ( ret < 0 ) {
		fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
		goto end;
	}

	if ( ret > 0 ) {
		fprintf( stdout, " - Recvd %d bytes: ", ret );
		for ( ii = 0; ii < ret; ii ++ ) {
			fputc( buf[ ii ], stdout );
		}
		fputs( "\n", stdout );
	}

	if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {
		errexit( 0, gnutls_strerror( ret ) );
	}
#else
	//send across socket
	if ( !socket_tcp_send ( &s, (uint8_t *)GetMsg, strlen(GetMsg) ) )
		errexit (0, "Couldn't send TCP packet... " );
	
	//recv socket	
	if ( !socket_tcp_recv ( &s, msg, (int *)&len ) )
		errexit (0, "Couldn't send TCP packet... " );

	//Hey, here's our message
	write( 2, msg, len );
#endif

end:
	socket_close( &s );
	gnutls_deinit( session );
	gnutls_certificate_free_credentials( xcred );
	gnutls_global_deinit();	
	return 0;
}

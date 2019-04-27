/* tlscli.c */
#include "vendor/single.h"
#include <gnutls/gnutls.h>
#define PROG "cx"

#define RUN(c) \
 (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)

#define errexit(c, ...) do { fprintf( stderr, "%s: ", PROG ); fprintf( stderr, __VA_ARGS__); return c; } while(0)

#define errdie(c) if ( c < 0 ) { fprintf( stderr, PROG ": %s\n", gnutls_strerror( c )); return c; }

#if 0
 #define PORT 80
 #define HOSTNAME "https.cio.gov"
#endif
#if 0
 #define PORT 80
 #define LOCATION "/article2/0,2817,2471051,00.asp"
 #define HOSTNAME "ramarcollins.com"
 #define NOHTTPS
#else
 #define PORT 443
 //#define LOCATION "/article2/0,2817,2471051,00.asp"
 //#define HOSTNAME "pcmag.com"
 #define LOCATION "/"
 #define HOSTNAME "deep909.com"
#endif

#ifndef HOSTNAME
 #error 'No hostname defined.'
#endif


Option opts[] = {
	{ "-s", "--site", "desc of site option", 's' }
	{ "-u", "--urlpath", "the url path", 's' }
 ,{ "-h", "--help", "Show help" }
 ,{ .sentinel = 1 }
};

 
int main (int argc, char *argv[]) {

	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	char *site = opt_get( opts, "--site" ).s;	
	char *path = opt_get( opts, "--urlpath" ).s;	

	int err, ret, sd, ii, type, len;
	unsigned int status;
	Socket s = { .server   = 0, .proto    = "tcp" };
	gnutls_session_t session;
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
	char buf[ 4096 ] = { 0 }, *desc = NULL;
	char msg[ 32000 ] = { 0 };
	char GetMsg[2048] = { 0 };	
	char GetMsgFmt[] = 
		"GET " LOCATION " HTTP/1.1\r\n"
		"Host: %s\r\n\r\n"
	;

	//nsprintf( HOSTNAME );
	//nsprintf( LOCATION );

	if ( RUN( !gnutls_check_version("3.4.6") ) ) { 
		errexit( 0, "GnuTLS 3.4.6 or later is required for this example." );	
	}

	//Is this needed?
	if ( RUN( ( err = gnutls_global_init() ) < 0 ) ) {
		errexit( err, gnutls_strerror( err ));
	}

	if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 )) {
		errexit( err, gnutls_strerror( err ));
	}

	if ( RUN( (err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )) {
		errexit( err, gnutls_strerror( err ));
	}
	/*
	//Set client certs this way...
	gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
	*/	

	//Initialize the session
	char *hostname = HOSTNAME;
	int port = PORT;

	//pack a message
	if ( port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, hostname );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s://%s:%d", "https", hostname, port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", hostname, port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, hbbuf );
	}

	//???
	fprintf( stderr, "%s\n", GetMsg ); //exit( 0 );
 	
	//Do socket connect (but after initial connect, I need the file desc)
	if ( RUN( !socket_connect( &s, hostname, port ) ) ) {
		errexit (0, "Couldn't connect to site... " );
	}

#ifdef NOHTTPS
	//send across socket
	if ( !socket_tcp_send ( &s, (uint8_t *)GetMsg, strlen(GetMsg) ) )
		errexit (0, "Couldn't send TCP packet... " );
	
	//recv socket	
	if ( !socket_tcp_recv ( &s, msg, (int *)&len ) )
		errexit (0, "Couldn't send TCP packet... " );

#else
	//Initialize gnutls and set things up
	if ( RUN( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 )) {
		errexit( err, gnutls_strerror( err ));
	}

	if ( RUN( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, hostname, strlen(hostname)) ) < 0)) {
		errexit( err, gnutls_strerror( err ));
	}

	if ( RUN( (err=gnutls_set_default_priority( session ) ) < 0) ) {
		errexit( err, gnutls_strerror( err ));
	}
	
	if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0) ) {
		errexit( err, gnutls_strerror( err ));
	}

	gnutls_session_set_verify_cert( session, hostname, 0 );
	//fprintf( stderr, "s.fd: %d\n", s.fd );
	gnutls_transport_set_int( session, s.fd );
	gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	//This is ass ugly...
	do {
		RUN( ret = gnutls_handshake( session ) );
	} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

	if ( RUN( ret < 0 ) ) {
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

	if (RUN( ( err = gnutls_record_send( session, GetMsg, len ) ) < 0 ))
		errexit( 0, "GnuTLS 3.4.6 or later is required for this example." );	

	if (RUN( (ret = gnutls_record_recv( session, buf, sizeof(buf))) == 0 )) {
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
		fprintf( stdout, "Recvd %d bytes:\n", ret );
		fflush( stdout );
		write( 1, buf, ret );
		fflush( stdout );
		fputs( "\n", stdout );
	}

	if (RUN((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 )) {
		errexit( 0, gnutls_strerror( ret ) );
	}
#endif

	//Hey, here's our message
	write( 2, msg, len );

end:
	socket_close( &s );
	gnutls_deinit( session );
	gnutls_certificate_free_credentials( xcred );
	gnutls_global_deinit();	
	return 0;
}

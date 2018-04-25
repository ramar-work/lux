#include "single.h"

#if 0
#ifdef OBSIDIAN_SSL
 #include <axTLS/ssl.h>
#endif
#endif

/* ----------------------------------- *
	Test keys for different API services 
	------------------------------------
	Meetup - 62a2f1d383c1772421177f14121616


	What do I want?
	---------------
	Get successfull JSON back from a cli
	request.  This way I can manipulate it.

	I also can start writing tools that 
	will make my idea look okay.
 * ----------------------------------- */

Option opts[] =
{
	{ "-s", "--site",     "Grab something from this site.", 's'  },
	{ NULL, "--verbose",  "Be verbose",'s' },
	{ .sentinel = 1 }
};


Socket s = 
{
	.server   = 0,
	.proto    = "tcp",
	.port     = 2000,
	.hostname = "localhost"
};


const char PostMsg[] = 
	"Content-Length\r\n\r\n<h2>Content</h2>";

const char GetMsg[]  = 
	"GET / HTTP/1.1\r\nHost: ramarcollins.com\r\nContent-Type: text/html\r\n\r\n";


int main (int argc, char *argv[])
{
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	int a = 64000;
	uint8_t msg[ (const int)a ];
	memset( msg, 0, a );

#ifdef OBSIDIAN_SSL
	SSL_CTX *ssl_ctx = NULL;
	SSL *ssl = NULL;
	char **ca_cert, **cert;
	int cert_size, ca_cert_size;
	int cert_index=0, ca_cert_index=0;

	//Create an SSL context
	ssl_ctx_new( 0 /*Or options here*/, SSL_DEFAULT_CLNT_SESS );
	cert_size = ssl_get_config( SSL_MAX_CERT_CFG_OFFSET );
	ca_cert_size = ssl_get_config( SSL_MAX_CERT_CFG_OFFSET );
	ca_cert = (char **)calloc( 1, sizeof( char *) * ca_cert_size );
	cert = (char **)calloc( 1, sizeof( char *) * cert_size );

	for ( int i=0; i < cert_index; i++ )
	{
		if ( ssl_obj_load( ssl_ctx, SSL_OBJ_X509_CERT, cert[i], NULL ) )
			return err( 0, "Certificate  %s is undefined.\n", cert[ i ] );
	}

	for ( int i=0; i < ca_cert_index; i++ )
	{
		if ( ssl_obj_load( ssl_ctx, SSL_OBJ_X509_CERT, ca_cert[i], NULL ) )
			return err( 0, "Certificate  %s is undefined.\n", ca_cert[ i ] );
	}

	free( cert );
	free( ca_cert );
#endif

	//fprintf( stderr, "%s\n", SSL_OBJ_X509_CERT );

	//make a socket
	if ( !socket_connect( &s, "ramarcollins.com", 80 ) )
		errexit (0, "Couldn't connect to site... " );

#ifdef OBSIDIAN_SSL
	ssl = ssl_client_new( ssl_ctx, s.fd, NULL, 0, 0 );
#endif

	//send across socket
	if ( !socket_tcp_send ( &s, (uint8_t *)GetMsg, strlen(GetMsg) ) )
		errexit (0, "Couldn't send TCP packet... " );
	
	//recv socket	
	if ( !socket_tcp_recv ( &s, msg, (int *)&a ) )
		errexit (0, "Couldn't send TCP packet... " );

	//Hey, here's our message
	write( 2, msg, a );
#ifdef OBSIDIAN_SSL
	ssl_ctx_free( ssl_ctx );
#endif
	return 0;
}

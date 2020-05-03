/*server-test.c*/
#include "server.h"
#include "socket.h"
#include "ctx-http.h"
#include "ctx-https.h"
#define PORT 2020
#define CTX 0 
#define TESTDIR "tests/server/"

int filter_test( struct HTTPBody *rq, struct HTTPBody *rs, struct config *config, struct host *host );


int read_test( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	const uint8_t body[] =
	 "GET /etc/2001-06-07?index=full&get=all HTTP/1.1\r\n"
	 "Content-Type:"" text/html\r\n"
	 "Host: hellohellion.com:3496\r\n"
	 "Upgrade:"     " HTTP/2.0, HTTPS/1.3, IRC/6.9, RTA/x11, websocket\r\n"
	 "Text-trapper:"" Nannybot\r\n\r\n"
	;
	return 1;
}

int write_test( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	const uint8_t body[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 11\r\n\r\n"
		"<h2>Ok!</h2>"
	;
	return 1;
}

void create_test( void **p ) {
}

int filter_test( struct HTTPBody *rq, struct HTTPBody *rs, struct config *config, struct host *host ) {
	char err[2048] = {0};
	char message[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";

	http_set_status( rs, 200 );
	http_set_ctype( rs, "text/html" );
	http_set_content( rs, (uint8_t *)message, strlen( message ) ); 

	if ( !http_finalize_response( rs, err, sizeof( err ) ) ) {
		return http_set_error( rs, 500, err );
	}

	return 1; 
}


void connect_and_send_msg ( const char *message ) {
	//Send a message to the default
	char err[2048] = {0};
	struct sockAbstr sa;
	if ( !open_connecting_socket( &sa, err, sizeof(err) ) ) {

	}

#if 0
	if ( send( sa.fd, message, strlen( message ) ) == -1 ) {

	}
#endif
}


//Filters
struct filter filters[] = {
	{ "test", filter_test },
	{ NULL }
};


//Contexts should be tested out most any way you can
struct senderrecvr contexts[] = {
	{ read_gnutls, write_gnutls, create_gnutls, NULL, pre_gnutls, post_gnutls },
#if 0
	{ read_notls, write_notls, create_notls },
	{ read_test, write_test, create_test },
#endif
 	{ NULL }
};


//Runs the server loop
int server_loop ( int type ) {
	struct sockAbstr su = {0};
	struct senderrecvr *ctx = &contexts[ type ];
	int fd, status, port = PORT;
	char err[ 2048 ];
	ctx->init( &ctx->data );
	ctx->config = TESTDIR "config.lua";
	ctx->filters = filters;
	populate_tcp_socket( &su, &port );

	//Open a socket
	if ( !open_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}

	//Accept the connection
	if ( !accept_listening_socket( &su, &fd, err, sizeof(err) ) ) {
		FPRINTF( "socket %d could not be marked as non-blocking\n", fd );
		return 0;
	}

	//Handle the request
	if ( !srv_response( fd, ctx ) ) {
		fprintf( stderr, "Error in response generation.\n" );
		return 0;
	}

	//Close a socket
	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "Couldn't close parent socket. Error: %s\n", err );
		return 0;
	}

	return 1;
}


int main (int argc, char *argv[]) {

	//All of the different contexts should get tested here (if it frees its' fine)
	//The filter should be the most basic thing there is...
	server_loop( CTX );

	return 0;
}

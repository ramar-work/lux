#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "hypno/zmq.h"

const char response[] = 
	"HTTP/1.1 200 OK\r\n"
	"Date: Sun, 10 Oct 2010 23:25:07 GMT\r\n"
	"Content-Length: 12\r\n"
	"Content-Type: text/html\r\n\r\n"
	"Hello world!"
	;

#define PORT "5555"

int main (int argc, char *argv[]) {
	void *ctx = zmq_ctx_new();
	void *sk = zmq_socket( ctx, ZMQ_STREAM );
	int rc = zmq_bind( sk, "tcp://*:" PORT );
	fprintf( stderr, "status was: %d\n", rc );

	if ( !rc ) 
		fprintf( stderr, "started a server on " PORT  "...\n" );

	while ( 1 ) {
		char req[100] = {0};
		int br = zmq_recv( sk, req, 99, 0 );
		printf( "Received Hello\n" );
		int maxlen = ( br > 99 ) ? 99 : br;
		req[ maxlen ]='\0';
		write( 2, req, maxlen );
		//Since this is non-blocking, you could just keep on receiving
		zmq_send( sk, response, strlen(response), 0 );
	}
	return 0;
}

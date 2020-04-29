#include "ctx-test.h"

//Sends a message over HTTP
int write_static ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	//write (write all the data in one call if you fork like this) 
	const char http_200[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";
	if ( write( fd, http_200, strlen(http_200)) == -1 ) {
		FPRINTF( "Couldn't write all of message...\n" );
		//close(fd);
		return 0;
	}

	return 0;
}


//Read a message (that we just discard)
int read_static ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message...\n" );
		close(fd);
		return 0;
	}
	return 0;
}

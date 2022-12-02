#include "../socket.h"
#include "../http.h"

//Sends a message over HTTP
int write_static ( int fd, zhttp_t *rq, zhttp_t *rs, void *p ) {
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
int read_static ( int fd, zhttp_t *rq, zhttp_t *rs, void *p ) {
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


int main (int argc, char *argv[] ) {
	const int port = 4001;
	char err[ 2048 ] = {0};
	struct sockAbstr server, client;
	memset( &server, 0, sizeof( struct sockAbstr ) );
	memset( &client, 0, sizeof( struct sockAbstr ) );
	server.port = &port;

	//Test the server with itself...
	if ( !open_listening_socket( &server, err, sizeof(err) ) ) {
		fprintf( stderr, "%s: %s\n", __FILE__, err );
		return 1;
	}

	//A fork needs to be created for the listener.
	int opened = 0;
	if ( 1 ) {
		while ( 1 ) {
			if ( !opened ) {
				struct sockAbstr *status = NULL;
				status = open_connecting_socket( &client, err, sizeof(err) );
				if ( !status ) {
					fprintf( stderr, "%s: %s\n", __FILE__, err );
					//close the child and exit
					return 1;
				}
				opened = 1;
				continue;
			}

			//Read off whatever you could read.
			while ( 1 ) {

			}

			//Close if the read was successful.
			if ( !close_connecting_socket( &client, err, sizeof(err) ) ) {
				fprintf( stderr, "%s: %s\n", __FILE__, err );
				return 1;
			}
		}
	}
 
	if ( !close_listening_socket( &server, err, sizeof(err) ) ) {
		fprintf( stderr, "%s: %s\n", __FILE__, err );
		return 1;
	}
	return 0;
}

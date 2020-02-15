#include "socket.h"

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

/*server-test.c*/
#include "server.h"
#include "socket.h"
#define PORT 2020

//Runs the server loop
int server_loop ( int type ) {
	struct sockAbstr su;
	struct senderrecvr *ctx = NULL;
	int fd, status, port = PORT;
	char err[ 2048 ];
	populate_tcp_socket( &su, &port );

	//Open a socket
	if ( !open_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}

	//Accept
	if ( !( fd = handle_accept( &su, err, sizeof( err )) ) ) {
		fprintf( stderr, "%s\n", err );
		return 0;
	}

	//Handle the request
	if ( !handle_tcp_socket( fd, ctx ) ) {
		fprintf( stderr, "Error in TCP socket handling.\n" );
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
	server_loop( 0 );

	return 0;
}

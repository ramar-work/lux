
//Runs the server loop
int server_loop ( ) {
	struct sockAbstr su;
	int port = 2020;
	populate_tcp_socket( &su, &port );

	if ( !open_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		close_listening_socket( &su, err, sizeof(err) );
		return 0;
	}

	//What kinds of problems can occur here?
	if ( !accept_loop1( &su, &models[0], err, sizeof(err) ) ) {
		fprintf( stderr, "Server failed. Error: %s\n", err );
		return 1;
	}

	if ( !close_listening_socket( &su, err, sizeof(err) ) ) {
		fprintf( stderr, "Couldn't close parent socket. Error: %s\n", err );
		return 1;
	}
	return 0;
}



int main (int argc, char *argv[]) {

	return 0;
}

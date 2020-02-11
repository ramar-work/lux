#include "../vendor/single.h"
#ifndef SOCKET_H
#define SOCKET_H
struct sockAbstr {
	int addrsize;
	int buffersize;
	int opened;
	int backlog;
	int waittime;
	int protocol;
	int socktype;
	int fd;
	int iptype; //ipv4 or v6
	int reuse;
	int family;
	int *port;
	struct sockaddr_in *sin;	
	void *ssl_ctx;
};

int read_from_socket ( int fd, uint8_t **b, void (*readMore)(int *, void *) );
void whatsockerr( int e ) ;

#endif

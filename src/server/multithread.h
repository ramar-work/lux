#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "server.h"
#include "../config.h"


// TODO: Eventually, this will be replaced by conn_t
struct threadinfo_t {
	int fd;
	char running;	
	pthread_t id;
	char ipaddr[ 128 ]; // Might be a little heavy..
	//struct senderrecvr *ctx;
	server_t *server;
#if 0
	struct timespec start;
	struct timespec end;
#endif	
};

int srv_multithread ( server_t * );

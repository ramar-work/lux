#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>

#include "server.h"
#include "../config.h"

typedef enum threadstatus_t {
	THREAD_AVAILABLE = 0,
	THREAD_ACTIVE,
	THREAD_INACTIVE
} threadstatus_t;

// TODO: Eventually, this will be replaced by conn_t
struct threadinfo_t {
	int fd;
	//char running;	
	threadstatus_t running;	
	pthread_t id;
	char ipaddr[ 128 ]; // Might be a little heavy..
	//protocol_t *ctx;
	server_t *server;
#if 0
	struct timespec start;
	struct timespec end;
#endif	
};

int srv_multithread ( server_t * );

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>

#include "server.h"
#include "../config.h"
#include "../logging/log.h"

#ifndef SRCSRV_MULTITHREAD_H
#define SRCSRV_MULTITHREAD_H

int srv_multithread ( server_t * );

#endif

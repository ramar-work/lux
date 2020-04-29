#include "server.h"
#include "socket.h"

int read_static ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_static ( int, struct HTTPBody *, struct HTTPBody *, void *);

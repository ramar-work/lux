#include "socket.h"
#include "server.h"
#include "../vendor/zhttp.h"

void create_notls ( void **p ); 
int read_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);

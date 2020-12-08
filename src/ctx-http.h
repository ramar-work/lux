#include "socket.h"
#include "server.h"
#include "../vendor/zhttp.h"
#include <signal.h>

void create_notls ( void **p ); 
int read_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);

//Added 12-08-20, signal handler for IO
//int read_ready ( int sn, const struct rt_sigaction *na, struct rt_sigaction *oa );

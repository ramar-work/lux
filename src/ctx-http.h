#include "socket.h"
#include "server.h"

void create_notls ( void **p ); 
int read_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
void free_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int accept_notls ( struct sockAbstr *, int *, void *, char *, int );

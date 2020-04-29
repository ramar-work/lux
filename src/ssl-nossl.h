#include "../vendor/single.h"
#include <stddef.h>
#include "socket.h"
#include "server.h"
#include "util.h"
#include "http.h"
#include <assert.h>

#define AC_WAGAIN 7

void * create_nossl();
int accept_nossl ( struct sockAbstr *, int *, char *, int );
int read_nossl( int, void *, uint8_t *, int );	
int write_nossl( int, void *, uint8_t *, int  );	
void destroy_nossl( void * );

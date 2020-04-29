#include "../vendor/single.h"
#include <axTLS/ssl.h>
#include <axTLS/os_port.h>
#include "ssl.h"

void * create_axtls();
int handshake_axtls( void *ctx, int fd );
int read_axtls( void *ctx );
int write_axtls( void *ctx );
void destroy_axtls ( void *ctx );

#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <stddef.h>
#include "socket.h"
#include "server.h"
#include "util.h"
#include <assert.h>

struct gnutls_abstr {
	gnutls_certificate_credentials_t x509_cred;
  gnutls_priority_t priority_cache;
	gnutls_session_t session;
#if 0
	const char *cafile;
	const char *crlfile; 
	const char *certfile;
	const char *keyfile;
#endif
};

int pre_gnutls ( int, struct config *, void ** );
int post_gnutls ( int, struct config *, void ** );
int read_gnutls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_gnutls ( int, struct HTTPBody *, struct HTTPBody *, void *);
void create_gnutls( void ** );
void free_gnults( void **p );

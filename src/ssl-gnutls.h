#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <stddef.h>
#include "ssl.h"
#include "socket.h"
#include "server.h"
#include "util.h"
#include <assert.h>

struct gnutls_abstr {
	gnutls_certificate_credentials_t x509_cred;
  gnutls_priority_t priority_cache;
	gnutls_session_t session;
	const char *cafile;
	const char *crlfile; 
	const char *certfile;
	const char *keyfile;
};

int accept_gnutls ( struct sockAbstr *, int *, void *, char *, int );
int read_gnutls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_gnutls ( int, struct HTTPBody *, struct HTTPBody *, void *);
void free_gnutls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *);
void create_gnutls( void ** );


int handshake_gnutls( void *, int );

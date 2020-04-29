#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <stddef.h>
#include "ssl.h"
#include "socket.h"
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

void * create_gnutls();
int accept_gnutls ( struct sockAbstr *, int *, char *, int );
int handshake_gnutls( void *, int );
int read_gnutls( void * );
int write_gnutls( void * );
void destroy_gnutls( void * );

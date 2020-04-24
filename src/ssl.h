#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <stddef.h>

#ifndef SSL_H
#define SSL_H

struct SSLContext {
	void *read;	
	void *write;	
	void *data;
	int *fd;
};

struct gnutls_abstr {
	gnutls_certificate_credentials_t x509_cred;
  gnutls_priority_t priority_cache;
	const char *cafile;
	const char *crlfile; 
	const char *certfile;
	const char *keyfile;
};

void open_ssl_context( struct gnutls_abstr * );

#endif

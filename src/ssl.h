#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <stddef.h>
#include "socket.h"

#ifndef SSL_H
#define SSL_H

#if 1
	//#define CAFILE   "/etc/ssl/certs/ca-certificates.crt"
	#define FPATH "tlshelp/"
	#define KEYFILE  FPATH "x509-server-key.pem"
	#define CERTFILE FPATH "x509-server.pem"
	#define CAFILE   FPATH "x509-ca.pem"
	#define CRLFILE  FPATH "crl.pem"
#else
	#define FPATH "tlshelp/collinshosting-final/"
	#define KEYFILE  FPATH "x509-collinshosting-key.pem"
	#define CERTFILE FPATH "x509-collinshosting-server.pem"
	#define CAFILE   FPATH "collinshosting_com.ca-bundle"
	#define CRLFILE  FPATH "crl.pem"
#endif

struct SSLContext {
	void *read;	
	void *write;	
	void *data;
	int *fd;
};

//void open_ssl_context( struct gnutls_abstr * );

#endif

/* ------------------------------------------- * 
 * ctx-https.h
 * ========
 * 
 * Summary 
 * -------
 * Header for HTTPS functions.
 *
 * Usage
 * -----
 * Soon to come...
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - 
 * ------------------------------------------- */
#ifndef NO_HTTPS_SUPPORT
#include <sys/stat.h>
#include <stddef.h>
#include <zwalker.h>
#include <ztable.h>
#include "../socket.h"
#include "../server.h"
#include "../util.h"

#include "../config.h"

#ifdef INCLUDE_GNUTLS 
 #include <gnutls/gnutls.h>
#endif

#ifndef CTXHTTPS_H
#define CTXHTTPS_H

#if 0
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
#endif

#ifdef INCLUDE_GNUTLS
struct gnutls_abstr {
	gnutls_certificate_credentials_t x509_cred;
  gnutls_priority_t priority_cache;
	gnutls_session_t session;
	char sniname[ 4096 ]; // The SNI name?
#if 0
	const char *cafile;
	const char *crlfile; 
	const char *cert_file;
	const char *key_file;
#endif
};
#endif

const int pre_gnutls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

const int post_gnutls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

const int read_gnutls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

const int write_gnutls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

void create_gnutls( void ** );

void free_gnults( void **p );
#endif
#endif

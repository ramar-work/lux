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
#include <sys/stat.h>
#include <stddef.h>
#include <zwalker.h>
#include <ztable.h>
#include "../server/server.h"
#include "../util.h"
#include "../config.h"

#if SENDFILE_ENABLED 
 #include <sys/sendfile.h>
#endif

#if !defined(DISABLE_TLS) && !defined(CTXHTTPS_H)
 #include <gnutls/gnutls.h>
 #define CTXHTTPS_H
 #define CTXHTTPS_SNI_LENGTH 256

// TODO: Can probably trim this even more
struct gnutls_abstr {
	gnutls_certificate_credentials_t *cbob;
	gnutls_session_t session;
	char sniname[ CTXHTTPS_SNI_LENGTH ]; // The SNI name?
};

const int pre_gnutls ( server_t *, conn_t * );
const int post_gnutls ( server_t *, conn_t * );
const int read_gnutls ( server_t *, conn_t * );
const int write_gnutls ( server_t *, conn_t * );
int create_gnutls( server_t * );
void free_gnutls( server_t * );
#endif

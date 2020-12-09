/* ------------------------------------------- * 
 * ctx-https.h
 * ------------
 * Header for HTTPS functions.
 *
 * Usage
 * -----
 * Soon to come...
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
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

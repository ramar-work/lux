/* ------------------------------------------- * 
 * http.h 
 * ======
 * 
 * Summary 
 * -------
 * Handle any web request from Lua (w/o cURL).
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * TODO
 * ----
 * - Only handles GET right now.  Needs other methods.
 * - Consider merging with zhttp to enable packaging responses.
 * - Allow alternate SSL backends. (at least OpenSSL)
 * - Test with http://etc.com:2000 (port numbers)
 * 
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <gnutls/gnutls.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <zwalker.h>
#include <zhttp.h>
#include "../lua.h"

#ifndef WEB_H
#define WEB_H

typedef struct wwwResponse {
	int status, len, clen, chunked;
	uint8_t *data, *body;
	char *redirect_uri;
	char ctype[ 1024 ];
	char err[ 1024 ];
	char ipv4[ INET6_ADDRSTRLEN ];
	char ipv6[ 1024 ];
} wwwResponse;

typedef struct stretchBuffer {
	int len;
	uint8_t *buf;
} Sbuffer;

extern struct luaL_Reg http_set[];
#endif

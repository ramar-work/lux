/* ------------------------------------------- * 
 * session.h
 * =========
 * 
 * Summary 
 * -------
 * -
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
#include <zdb.h>
#include <zhttp.h>
#include <ztable.h>
#include "../lua.h"
#include "../config.h"
#include "rand.h"

#if !defined(DISABLE_TLS) && !defined(LSESSION_H)
 #include <gnutls/crypto.h>
 #define LSESSION_H
extern struct luaL_Reg session_set[];
#endif

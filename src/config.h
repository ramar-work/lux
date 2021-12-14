/* ------------------------------------------- * 
 * config.h
 * ========
 * 
 * Summary 
 * -------
 * General configuration at compile time
 * Under construction.
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

//How long should each client be allowed to persist?
//When the timeout is reached, send a 503 back to the client
//(perhaps we could further customize this, see LOAD_TOO_HIGH_STATUS )
#define CLIENT_REQUEST_TIMEOUT 5

#define LOAD_TOO_HIGH_STATUS 503

//Where are the default htdocs?
#define HTDOCS_DIR /var/www

//How often should a process poll for new data?
#define POLL_INTERVAL 100000000

//
#define CTX_READ_SIZE 1024
#define CTX_WRITE_SIZE 1024

#if 0
//Activate / deactivate database engines
#define NOMYSQL_H
#define NOPGSQL_H
#define NOSQLITE_H

//Forks or threads
#define FORK_H
//#define HFORK_H
//#define HTHREAD_H

//Enable or disable certain filters
#define USEFILTER_C
#define USEFILTER_LUA
#define USEFILTER_STATIC
#define USEFILTER_ECHO
#endif

//Enable or disable content types
#define INCLUDE_XML_SUPPORT
#define INCLUDE_JSON_SUPPORT

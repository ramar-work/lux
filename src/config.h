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

//How many clients until the main thread cleans up
#define CLIENT_MAX_SIMULTANEOUS 64

//Default status for too many connections
#define LOAD_TOO_HIGH_STATUS 503

//Where are the default htdocs?
#define HTDOCS_DIR /var/www

//How often should a process poll for new data?
#define POLL_INTERVAL 1000000

//Default read buffer size
#ifndef ZHTTP_PREAMBLE_SIZE
 #define ZHTTP_PREAMBLE_SIZE 2048
#endif

//Default read buffer size
#ifndef CTX_READ_SIZE
 #define CTX_READ_SIZE 4096 
#endif

//Default write buffer size
#define CTX_WRITE_SIZE 1024

//Default stack size of each new thread 
#define STACK_SIZE 100000

//Default thread limit
#define MAX_THREADS 1024 

//Default listener backlog
#define BACKLOG 4096

//Enable XML support
#define INCLUDE_XML_SUPPORT

//Enable JSON support
#define INCLUDE_JSON_SUPPORT

//Default error log file location
#define ERROR_LOGFILE "/var/log/hypno-error.log"

//Default access log file location
#define ACCESS_LOGFILE "/var/log/hypno-access.log"

//Forks or threads
#if 0
#undef HBLOCK_H
#undef HFORK_H
#undef HTHREAD_H
#endif

#if 0
//Activate / deactivate database engines
#define NOMYSQL_H
#define NOPGSQL_H
#define NOSQLITE_H

//Enable or disable certain filters
#define USEFILTER_C
#define USEFILTER_LUA
#define USEFILTER_STATIC
#define USEFILTER_ECHO
#endif


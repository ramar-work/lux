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

//How long should each client be allowed to persist
#define CLIENT_REQUEST_TIMEOUT 5

//Where are the default htdocs?
#define HTDOCS_DIR /var/www

//How often should a process poll for new data?
#define POLL_INTERVAL 100000000

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

//Enable or disable content types
#define INCLUDE_XML_SUPPORT
#define INCLUDE_JSON_SUPPORT

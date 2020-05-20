#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "luabind.h"
#if 0
 #include <mysql.h>
#endif
#if 0
 #include <postgresql.h>
#endif
#if 1
#include <sqlite3.h>
#endif

#ifndef HDATABASE_H
#define HDATABASE_H

void *db_open( const char *, char *, int ) ;
void *db_close( void **, char *, int );
Table *db_exec( void *, const char *, void **, char *, int ) ;
Table *db_to_table ( const char *filename, const char *query );

#endif

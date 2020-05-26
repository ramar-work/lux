#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "luabind.h"

#ifndef SKIPSQLITE3_H
 #include <sqlite3.h>
#endif

#ifndef SKIPMYSQL_H
 #include <mysql.h>
#endif

#ifndef SKIPPGSQL_H
 #include <postgresql.h>
#endif

#ifndef HDATABASE_H
#define HDATABASE_H

void *db_open( const char *, char *, int ) ;
void *db_close( void **, char *, int );
Table *db_exec( void *, const char *, void **, char *, int ) ;
Table *db_to_table ( const char *filename, const char *query );

#endif

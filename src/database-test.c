#include "db-sqlite.h"

#define TESTDIR "tests/database/"

struct dbtest { const char *db, *type, *query; } dbtests[] = {
	{ TESTDIR "ges.db", "SELECT", "SELECT * FROM general_election LIMIT 2" }
,	{ TESTDIR "ges.db", "SELECT", "SELECT * FROM general_election LIMIT 10" }
//,	{ "tests/ges.db", "SELECT", "SELECT * FROM general_election LIMIT 10" }
,	{ NULL, 0, 0 }
};

int main (int argc, char *argv[] ) {
	struct dbtest *db = dbtests;
	while ( db->db ) {	
		const char *dbstr = db->db;
		char err[ 2048 ] = {0};
		void *handle = NULL;
		Table *t = NULL;

		fprintf( stderr, "Using: %s\n", dbstr );
		fprintf( stderr, "DB handle at %p\n", handle );
		if ( !( handle = db_open( dbstr, err, sizeof(err) ) ) ) {
			fprintf( stderr, "COULD NOT OPEN DATABASE AT %s! %s\n", dbstr, err );
			db++;
			continue;
		}
		fprintf( stderr, "DB handle at %p\n", handle );

		fprintf( stderr, "Table at %p\n", t );
		if ( !( t = db_exec( handle, db->query, NULL, err, sizeof(err) ) ) ) {
			fprintf( stderr, "COULD NOT EXECUTE DATABASE QUERY: '%s'! %s\n", db->query, err );
			db++;
			continue;
		}

		fprintf( stderr, "Table now at %p\n", t );
		lt_dump( t );
		lt_free( t );
		free( t );

		if ( !( db_close( &handle, err, sizeof(err) ) ) ) {
			fprintf( stderr, "COULD NOT CLOSE DATABASE AT %s, %s\n", dbstr, err );
			db++;
			continue;
		}
		fprintf( stderr, "DB handle now at %p\n", handle );
		db++;
	}
	return 0;
}

/* ------------------------------------------- * 
 * db-sqlite.c
 * -----------
 * Functions for dealing with data interchange between zTable and sqlite3.
 *
 * Usage
 * -----
 * gcc -ldl -llua -o database vendor/single.o database.c && ./config
 * 
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
#include "db-sqlite.h"

int db_check_query ( const char *query, char *err, int errlen ) {
	if ( !query ) {
		snprintf( err, errlen, "No query specified." );
		return 0;
	}

	//trim the query of whitespace, and check the last position for ';'
	//for some reason, sqlite does not do well when that is not there...
	if ( !query ) {
		snprintf( err, errlen, "No query specified." );
		return 0;
	}

	return 1;
}



void *db_open( const char *string, char *err, int errlen ) {
	sqlite3 *ptr = NULL;
	if ( sqlite3_open( string, &ptr ) != SQLITE_OK ) {
		snprintf( err, errlen, "%s", sqlite3_errmsg( ptr ) ); 
		return NULL;	
	}
	return ptr;
}


void *db_close( void **ptr, char *err, int errlen ) {
	if ( sqlite3_close( *ptr ) != SQLITE_OK ) {
		snprintf( err, errlen, "%s", sqlite3_errmsg( *ptr ) ); 
		return NULL;	
	}
	*ptr = NULL;
	return (int *)1;
}


zTable *db_exec( void *ptr, const char *query, void **records, char *err, int errlen ) { 
	int columnCount = 0, status = 0;
	zTable *t = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *unused = NULL;
	const char *columns[ 127 ] = { 0 };

	if ( !( t = malloc( sizeof( zTable ) ) ) || !lt_init( t, NULL, 1024 ) ) {
		snprintf( err, errlen, "%s\n", "COULD NOT ALLOCATE SPACE FOR TABLE!" ); 
		return NULL;
	}

	if ( !ptr ) {
		snprintf( err, errlen, "%s\n", "NO DATABASE HANDLE SPECIFIED!" ); 
		return NULL;
	}

	if ( !query || strlen( query ) < 3 ) {
		snprintf( err, errlen, "%s\n", "QUERY NOT SPECIFIED!" ); 
		return NULL;
	}

	//Prepare the statment
	if ( ( status = sqlite3_prepare_v2( ptr, query, -1, &stmt, &unused ) ) != SQLITE_OK ) {
		snprintf( err, errlen, "SQLITE3 PREPARE ERROR: %s\n", sqlite3_errmsg( ptr ) );
		return NULL;
	}

	//Get the column names (try to clone just once)
	if ( ( columnCount = sqlite3_column_count( stmt ) ) > 127 ) {
		snprintf( err, errlen, "RESULT SET COLUMN COUNT TOO LARGE (>127)\n" );
		return NULL;
	}

	//Start stepping through
	fprintf( stderr, "SQL Column Count: %d\n", columnCount );
	for ( int i=0; i < columnCount; i++ ) {
		columns[ i ] = sqlite3_column_name( stmt, i );
	}

	//Save each result
	if ( columnCount ) {
		lt_addtextkey( t, "results" );
		lt_descend( t );
		for ( int row = 0; sqlite3_step( stmt ) != SQLITE_DONE; row++ ) {
			lt_addintkey( t, row );
			lt_descend( t );

			for ( int i=0, len; i < sqlite3_data_count( stmt ); i++ ) {
				const uint8_t *bytes = sqlite3_column_blob( stmt, i );

				if ( !lt_addtextkey( t, columns[ i ] ) ) {
					snprintf( err, errlen, "FAILED TO ADD TEXT KEY\n" );
					return NULL;
				}
			
				if ( !( len = sqlite3_column_bytes( stmt, i )) ) {
					lt_addtextvalue( t, "" );
				}
				else {
					if ( !lt_addblobdvalue( t, bytes, len ) ) {
						snprintf( err, errlen, "FAILED TO ADD DATABASE VALUE AT ROW %d:COL %d (%s)\n", i, row, columns[ i ] );
						return NULL;
					}
				}

				lt_finalize( t );
			}
			lt_ascend( t );
		}

		//Finalize the table.
		lt_ascend( t );
		lt_lock( t );
	}

	//Need this to close out the database.
	sqlite3_finalize( stmt );
	return t;
}


zTable * db_to_table ( const char *filename, const char *query ) {
	return 0;
}





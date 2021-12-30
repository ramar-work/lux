/*log.c*/
#include "log.h"

int f_open( char *name, void **d ) {
	FILE *f = fopen( name, "a" );
	return ( ( *d = f ) != NULL );	
}

int f_close( void *d ) {
	return 1;	
}

int f_write( void *d ) {
	FILE *f = (FILE *)d;
	return fclose( f ) != -1;
}

char * f_handler() {
	return strerror( errno );	
}


int sqlite3_log_open ( char *name, void **d ) {
	sqlite3 *ppdb;

	//open a sqlite3 handle
	if ( sqlite3_open( name, &ppdb ) != SQLITE_OK ) {
		return 0;
	}

	*d = (void *)ppdb;	
	return 1;
}


int sqlite3_log_close( void *d ) {
	return ( sqlite3_close( (sqlite3 *)d ) == SQLITE_OK ); 
}


int sqlite3_log_write( void *dd ) {
	return 1;
}

char * sqlite3_handler() {
	return strerror( errno );	
}


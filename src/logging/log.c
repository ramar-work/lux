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


//const struct timespec __interval__ = { 1, 0 };


// Sleep for a specified time
int nsleep ( long nanoseconds ) {
	return 0;
	// nanosleep( &__interval__, NULL );
}


int time_diff_sec ( struct timespec *begin, struct timespec *end ) {
	return end->tv_sec - begin->tv_sec;
}


long time_diff_nsec ( struct timespec *begin, struct timespec *end ) {
	return end->tv_nsec - begin->tv_nsec;
}



int time_format ( struct timespec *timestamp, char *buf, int buflen ) {
	// pass this in and let strftime interpret?
	// or time
	if ( !timestamp || !timestamp->tv_sec ) {
		return 0;
	}

	// copy time to string
	char ibuf[ 64 ] = {0};
	snprintf( ibuf, sizeof( ibuf ), "%ld\n", timestamp->tv_sec );

	ctime_r( &timestamp->tv_sec, buf );
#if 0
	// copy it to another structure
	struct tm tm;
	memset( &tm, 0, sizeof( struct tm ) );
	strptime( ibuf, "%s", &tm );

	// turn that into a formatted time
	strftime( buf, buflen, "%Y-%m-%d %H:%M:%S", &tm );
#endif
	return 1;
}




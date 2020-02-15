#include "util.h"

const char *words[] = {
	"arbutus",
	"TAMMYPIGEONBACON",
	"carousel",
	"3333242423824234!",
	"enterprise",
	"?something_last_way?",
	"El Segundo",
	NULL
};


int main ( int argc, char *argv[] ) {

	//Test append with a bunch of stuff for now...
	fprintf( stderr, "APPEND_TO_UINT8T: " );
	const char **ww = words;
	uint8_t *w = NULL;
	int wlen = 0;
	while ( *ww ) {
		w = append_to_uint8t ( &w, &wlen, (uint8_t *)*ww, strlen( *ww ) );
		if ( !w ) {
			fprintf( stderr, "FAILED - append_to_uint8t on '%s'\n", *ww );
			free( w );	
			break;	
		} 
		ww++;
	}

	if ( !append_to_uint8t( &w, &wlen, (uint8_t *)"\0", 1 ) ) {
		fprintf( stderr, "FAILED - failed to terminate appended string.\n" );
		free( w );	
		return 1;
	}

	fprintf( stderr, "SUCCESS - '%s'\n", (char *)w ); 
	free( w );
	return 0;

}

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

	//Test random character generation
	fprintf( stderr, "RANDOM CHARACTER GENERATION (static):\n" );
	char buf[ 24 ];
	char *a = srand_nums( buf, sizeof(buf) );
	fprintf( stderr, "random nums: %s\n", a );
	char *b = srand_letters( &buf, sizeof(buf) );
	fprintf( stderr, "random letters: %s\n", b );
	char *c = srand_chars( &buf, sizeof(buf) );
	fprintf( stderr, "random chars: %s\n", c );

	fprintf( stderr, "RANDOM CHARACTER GENERATION (dynamic):\n" );
	char *d = mrand_nums( 20 );
	fprintf( stderr, "random nums: %s\n", d );
	free(d);
	char *e = mrand_letters( 33 ); 
	fprintf( stderr, "random letters: %s\n", e );
	free(e);
	char *f = mrand_chars( 42 ); 
	fprintf( stderr, "random chars: %s\n", f );
	free(f);

	//Test append with a bunch of stuff for now...
	fprintf( stderr, "APPEND_TO_UINT8T:\n" );
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

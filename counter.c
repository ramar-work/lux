#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABS(i) \
	printf( &"\t\t\t\t\t\t\t\t\t\t"[10-i] )


int main (int argc, char *argv[]) {
	typedef struct { int a, b; } IntSet;

	IntSet ii[] = {
		{ 0, 3 },
		{ 0, 4 },
		{ 0, 2 },
		{ 0, 1 }
#if 0
#endif
	};

	//OK, this is it...
	int p = 0;
	while ( ++p <= (sizeof(ii)/sizeof(IntSet) ) ) {
		
		IntSet **intset = NULL;
		intset = malloc( sizeof(IntSet *) * p );
		memset( intset, 0, sizeof(IntSet *) * p );
		for ( int i=0; i<p; i++ ) {
			intset[ i ] = &ii[ i ];
		}
	
		IntSet **w = intset;
		int c = 0;
		//printf( "%d = [", p );
		while ( (*w)->a < (*w)->b ) {
			//Show me the status of the line in question
			//printf( "\n  %p => ", w ); TABS(c); printf( "%d, ", (*w)->a );
	
			//Move to the next block or build a sequence
			if ( c < (p - 1) ) {
				w++, c++;
				//printf( "[UP] from %d - %d =? %d", c-1, c, (*w)->a, (*w)->b );
				continue;
			}
			
			//Print a sequence
			//printf( "[HIGHEST] => sequence " );
			for ( int i=0; i<p; i++ ) {
				printf( "%d,", intset[i]->a );
			}

			//Increment the number 
			while ( 1 ) {
				(*w)->a++;
				//printf( "L%d %d == %d, STOP", c, (*w)->a, (*w)->b );
				if ( c == 0 )
					break;
				else { // ( c > 0 )
					if ( (*w)->a < (*w)->b ) 
						break;
					else {
						(*w)->a = 0;
						w--, c--;
					}
				}
			}
		}
		(*w)->a = 0;
		//printf( "\n]\n" );
	}

	return 0;
}

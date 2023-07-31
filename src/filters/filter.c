#include "filter.h"


struct filter * check_filter ( const struct filter *filters, char *name ) {
	while ( filters && filters->name ) {
		struct filter *f = ( struct filter * )filters;
		if ( f->name && strcmp( f->name, name ) == 0 ) {
			return f;
		}
		filters++;
	}
	return NULL;
}

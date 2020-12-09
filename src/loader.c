/* ------------------------------------------- * 
 * loader.c 
 * =========
 * 
 * Summary 
 * -------
 * Loads config files written with Lua into a structure that C can easily talk 
 * to.
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
 * No entries yet.
 * ------------------------------------------- */
#include "loader.h"

//Get integer value from a table
int loader_get_int_value ( zTable *t, const char *key, int notFound ) {
	int i = lt_geti( t, key );
	zhRecord *p = NULL;
	if ( i == -1 ) {
		return notFound;
	}

	if (( p = lt_ret( t, LITE_INT, i ))->vint == 0 ) {
		return notFound;
	}

	return p->vint;
}


//Get string value from a table
char * loader_get_char_value ( zTable *t, const char *key ) {
	int i = lt_geti( t, key );
	zhRecord *p = NULL;
	if ( i == -1 ) {
		return NULL;
	}

	if (( p = lt_ret( t, LITE_TXT, i ))->vchar == NULL ) {
		return NULL;
	}

	return p->vchar;
}


//Drop things back to zero
static int loader_check_eot( zKeyval *kv, int *depth ) {
	if ( *depth && kv->key.type == LITE_TRM )
		(*depth)--;
	else if ( *depth && kv->value.type == LITE_TBL ){
		(*depth)++;
	}
#if 0
	else if ( !(*depth) && kv->key.type == LITE_TXT && !strcmp( kv->key.v.vchar, n ) ) {
		if ( kv->value.type == LITE_TBL ) {
			(*depth)++;
		}
		return 0;
	}
#endif
	return ( *depth > 0 );
}


//Runs on tables...
static int loader_iterator( zKeyval *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;

#if 1
	//TODO: This really should be in single.c, but that code is everywhere...
	if ( !loader_check_eot( kv, &f->depth ) ) {
		return f->depth; //This is an unusual hack, but it works
	}
#else
	if ( f->depth && kv->key.type == LITE_TRM )
		f->depth--;
	else if ( f->depth && kv->value.type == LITE_TBL ){
		f->depth++;
	}

#if 1
	//This is a strange way of working with nicely stacked recursive tables
	if ( f->depth == f->end ) {
		return 0;
	}
#endif
#endif

	return f->exec( kv, i, p );
}


//Get an array of things
void ** loader_get_table_value( zTable *t, const char *key, int(*fp)(zKeyval *,int,void *)) {
	FPRINTF( "start populating from table.\n" );
	int i = 0; 
	void **p = NULL;
	struct fp_iterator fp_data = { 0, 1, &p, fp, t };

	//The index should be there and the type should be a table
	if ( ( i = lt_geti( t, key ) ) == -1 || lt_valuetypeat( t, i ) != LITE_TBL ) {
		return NULL;
	}

	//Finally, fp->depth should be zero when done, but starting at one may save time
	if ( !lt_exec_complex( t, ++i, t->count, &fp_data, loader_iterator ) ) {
		return p; 
	}

	FPRINTF( "done populating from table.\n" );
	return p;
}


//Copy iterator
static int copy_iterator( zKeyval *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p; 
	zTable **t = (zTable **)f->userdata;
	FPRINTF( "zTable at %s: %p\n", __func__, *t );
	FPRINTF( "Running copy_iterator.\n" );

	//like earlier, calculate until the depth is zero again
	if ( !loader_check_eot( kv, &f->depth ) ) {
		return f->depth; //This is an unusual hack, but it works
	}

	if ( kv->key.type == LITE_INT )
		lt_addintkey( *t, kv->key.v.vint );
	else if ( kv->key.type == LITE_TXT )
		lt_addtextkey( *t, kv->key.v.vchar );
	else if ( kv->key.type == LITE_BLB ) 
		lt_addblobkey( *t, kv->key.v.vblob.blob, kv->key.v.vblob.size );
	else if ( kv->key.type == LITE_TRM ) {
		lt_ascend( *t );
		//lt_finalize( *t );
		return 1;
	}

	if ( kv->value.type == LITE_INT )
		lt_addintvalue( *t, kv->value.v.vint );
	else if ( kv->value.type == LITE_TXT )
		lt_addtextvalue( *t, kv->value.v.vchar );
	else if ( kv->value.type == LITE_BLB ) 
		lt_addblobvalue( *t, kv->value.v.vblob.blob, kv->value.v.vblob.size );
	else if ( kv->value.type == LITE_FLT )
		lt_addfloatvalue( *t, kv->value.v.vfloat );	
	else if ( kv->value.type == LITE_USR )
		lt_addudvalue( *t, kv->value.v.vusrdata );
	else if ( kv->value.type == LITE_TBL ) {
		FPRINTF( "Got table...\n" );
		lt_descend( *t );
		//you could run the copy iterator here
		return 1;
	}

	lt_finalize( *t );
	FPRINTF( "Done running copy_iterator.\n" );
	return 1;
}


//Shallow copy
zTable *loader_shallow_copy ( zTable *t, int start, int end ) {
	//Finally, fp->depth should be zero when done, but starting at one may save time
	zTable *nt = malloc( sizeof ( zTable ) );
	lt_init( nt, NULL, 256 );
	FPRINTF( "zTable at %s: %p\n", __func__, nt );
	struct fp_iterator fp_data = { end - start, 1, &nt, NULL, t };	

	//Copy this
	if ( !lt_exec_complex( t, start, t->count, &fp_data, copy_iterator ) ) {
		lt_reset( t );
		lt_lock( nt );
		return nt; 
	}

	lt_reset( t );
	lt_lock( nt );
	return nt;
}


#if 0
//Set found keys
int loader_set_keys ( zTable *t, const struct rule *rule ) {
	return 0;
}


//If a key is necessary, we need to search for it
int loader_check_keys ( zTable *t, const struct rule *rule ) {
	return 0;
}
#endif


//Loading can be done from just about anything...
int loader_run ( zTable *t, const struct rule *rule ) {
	FPRINTF( "Initializing configuration.\n" );

	while ( rule->key ) {
		//Find the key in the table 	
		FPRINTF( "Searching for key '%s'.\n", rule->key );
		int ii = lt_geti( t, rule->key );
		if ( ii == -1 ) {
			FPRINTF( "Key '%s' not found.\n", rule->key );
			rule++;
			continue;	
		}

		//Set the key somehow (assuming that the user did things right)
		if ( !rule->type ) { 
			FPRINTF( "Type not set for rule key '%s'\n", rule->key );
		}
		else if ( *rule->type == 's' ) {
			FPRINTF( "Got value '%s' for rule key '%s'\n", *rule->v.s, rule->key ); 
			*rule->v.s = strdup( loader_get_char_value( t, rule->key ) ); 
		}
		else if ( *rule->type == 'i' ) { 
			FPRINTF( "Got value '%d' for rule key '%s'\n", *rule->v.i, rule->key ); 
			*rule->v.i = loader_get_int_value( t, rule->key, -99 ); 
		}
		//Save a table (you can send a function pointer)
		else if ( *rule->type == 't' ) {
			if ( !rule->handler )
				FPRINTF( "No handler for value '%p' at rule key '%s\n", *rule->v.t, rule->key );
			else { 
				FPRINTF( "Got value '%p' & function '%p' for rule key '%s'\n", 
					*rule->v.t, rule->handler, rule->key ); 
				*rule->v.t = loader_get_table_value( t, rule->key, rule->handler ); 
			}
		}
		else if ( *rule->type == 'x' ) {
			if ( !rule->handler )
				FPRINTF( "No handler for value '%p' at rule key '%s\n", *rule->v.t, rule->key );
			else { 
				FPRINTF( "Got value '%p' & function '%p' for rule key '%s'\n", 
					*rule->v.t, rule->handler, rule->key ); 
					
				struct fp_iterator f = { 0, 0, rule->v.t, rule->handler };
				int count = lt_counta( t, ii );
				lt_exec_complex( t, ii, ii + count, &f, rule->handler );
			}
		}
	#if 0
		//Save userdata
		else if ( *rule->type == 'u' ) { 
			FPRINTF( "Got value '%p' for rule key '%s'\n", *rule->v.i, rule->key ); 
			*rule->v.i = loader_get_int_value( t, rule->key ); 
		}
		//Save function pointers
		else if ( *rule->type == 'f' ) { 
			FPRINTF( "Got value '%p' for rule key '%s'\n", *rule->v.i, rule->key ); 
			*rule->v.i = loader_get_int_value( t, rule->key ); 
		}
	#endif
		else {
			FPRINTF( "Unknown type set for rule key '%s'\n", rule->key );
		} 
		
		rule++;
	}

	FPRINTF( "Done building configuration.\n" );
	return 1;
}


//Free loader values?
void loader_free ( const struct rule *rule ) {
	while ( rule->key ) {
		//Set the key somehow (assuming that the user did things right)
		if ( !rule->type ) { 
			FPRINTF( "Type not set for rule key '%s'\n", rule->key );
		}
		else if ( *rule->type == 's' ) {
			free( *rule->v.s );
		}
		else if ( *rule->type == 'i' ) { 
			;
		}
	#if 0
		//Save a table (you can send a function pointer)
		else if ( *rule->type == 't' ) { 
			FPRINTF( "Got value '%p' for rule key '%s'\n", *rule->v.i, rule->key ); 
			int len;
			*rule->v.i = loader_iterate( t, fp, rule->key, len ); 
		}
		//Save userdata
		else if ( *rule->type == 'u' ) { 
			FPRINTF( "Got value '%p' for rule key '%s'\n", *rule->v.i, rule->key ); 
			*rule->v.i = loader_get_int_value( t, rule->key ); 
		}
		//Save function pointers
		else if ( *rule->type == 'f' ) { 
			FPRINTF( "Got value '%p' for rule key '%s'\n", *rule->v.i, rule->key ); 
			*rule->v.i = loader_get_int_value( t, rule->key ); 
		}
	#endif
		else {
			FPRINTF( "Unknown type set for rule key '%s'\n", rule->key );
		}
	}
}


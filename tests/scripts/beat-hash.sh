#!/bin/bash -

# This loops through and finds specific hashes
# in a.lua
#
# single.c must be compiled with DEBUG_H in
# and LT_DUMPFULL in order for the tests to
# return the correct text.
#
# It is, unfortunately, a manual test right 
# now.  The hash returned in 'hash of '<x>' should
# match the hash in brackets in the block of code
# that precedes the 'hash of' lines.
#
for n in `seq 1 100`
do
	# Let's be a little bit more...
	./ho 2>&1 | \
		grep \
			-e 'falafel ->' \
			-e "hash of 'falafel'" \
			-e 'spaghetti.sauce ->' \
			-e "hash of 'spaghetti.sauce'" \
			-e 'spaghetti.pie.model.1 ->' \
			-e "hash of 'spaghetti.pie.model.1'" \
			-e 'falafel.view ->' \
			-e "hash of 'falafel.view'"
done


# The code here will have to be dropped in somewhere in tests.c
#	//These are additional tests
#	char *as = NULL;
#	as = "spaghetti.sauce";
#	yh = lt_get_long_i( &t, (uint8_t *)as, strlen( as ) ); 
#	printf( "hash of '%s' is %d\n", as, yh );
#
#	as = "spaghetti.pie.model.1";
#	yh = lt_get_long_i( &t, (uint8_t *)as, strlen( as ) ); 
#	printf( "hash of '%s' is %d\n", as, yh );
#
#	as = "falafel.view";
#	yh = lt_get_long_i( &t, (uint8_t *)as, strlen( as ) ); 
#	printf( "hash of '%s' is %d\n", as, yh );


#hash of 'falafel' is 21
#hash of 'spaghetti.sauce' is 2
#hash of 'spaghetti.pie.model.1' is 15
#hash of 'falafel.view' is 27

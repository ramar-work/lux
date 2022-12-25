/* ------------------------------------------- * 
 * cliutils.h
 * ==========
 *
 * Summary 
 * -------
 * Command line program utilities (just macros 
 * at the moment)
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * See LICENSE in the top-level directory for more information.
 *
 * Changelog
 * ---------
 * 
 * ------------------------------------------- */
#define ERRPRINTF(...) \
	fprintf( stderr, "%s: ", NAME ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fprintf( stderr, "%s", "\n" );

#define OPTEVAL(ARGV, S_OPT, L_OPT) \
	!strcmp( ARGV, S_OPT ) || !strcmp( ARGV, L_OPT )

#define OPTARG(ARGV, MATCH) \
	if ( !( --argc ) || !( *( ++argv ) ) ) \
		return eprintf( "Expected argument for " MATCH "!" );


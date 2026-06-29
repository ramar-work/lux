/* ------------------------------------------- * 
 * filter-dirent.c
 * ===========
 * 
 * Summary 
 * -------
 * Functions comprising the dirent filter for interpreting HTTP messages.
 *
 * Usage
 * -----
 * filter-dirent.c allows hypno to act as a directory server, in which the
 * server simply presents the user with a list of files for view or download. 
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "filter-dirent.h"

static const unsigned char templ[] = " \
<table> \
	<thead> \
		<th>Filename</th> \
		<th>Size</th> \
		<th>Type</th> \
		<th>Date Changed</th> \
		<th>Date Accessed</th> \
		<th>Date Modified</th> \
	</thead> \
	<tbody> \
		{{ #directories }} \
		<tr> \
			<td><a href=\"/\">{{ .name }}</a></td> \
			<td>{{ .size }}</td> \
			<td>{{ .filetype }}</td> \
			<td>{{ .date_changed }}</td> \
			<td>{{ .date_accessed }}</td> \
			<td>{{ .date_modified }}</td> \
		</tr> \
		{{ /directories }} \
	</tbody> \
</table> \
";

const int 
filter_dirent ( int fd, zhttp_t *req, zhttp_t *res, struct cdata *conn ) {
	const int pathlen = 2048;
	char err[ 2048 ], path[ pathlen ]; //= { 0 }, path[ (const int)pathlen ] = {0};
	struct dirent *dir = NULL;
	struct stat st;
	char *dirname = NULL;
	DIR *ds = NULL;
	zTable t;
	lt_init( &t, NULL, 1027 );

	//I hate GCC
	memset( err, 0, sizeof( err ) );
	memset( path, 0, pathlen );

	//Make sure a directory was specified
	if ( !( dirname = conn->hconfig->dir ) ) {
		return http_set_error( res, 500, "No directory specified" );
	}

	//If we fail to open, then the filter is no good
	if ( !( ds = opendir( dirname ) ) ) {
		return http_set_error( res, 404, "Page not found..." );
	}

	//Somehow, we need to get a number of entries before starting
	int index = 0;
	lt_addtextkey( &t, "directories" );
	lt_descend( &t );

	//Get directory listing
	while ( ( dir = readdir( ds ) ) ) {
		//Check config to hide . & ..
		if ( *dir->d_name == '.' ) continue;	

		//Save pathname	
		snprintf( path, pathlen, "%s/%s", dirname, dir->d_name );

		//Configure what happens if the server can't access a file 
		if ( stat( path, &st ) == -1 ) continue;

		//Make a new table
		lt_addintkey( &t, index++ );
		lt_descend( &t );

		//Add the file name...
		lt_addtextkey( &t, "name" ); 
		lt_addtextvalue( &t, dir->d_name );
		lt_finalize( &t );

		//check file type
		lt_addtextkey( &t, "filetype" ); 
		if ( S_ISREG( st.st_mode ) )
			lt_addtextvalue( &t, "file" );
		else if ( S_ISDIR( st.st_mode ) )
			lt_addtextvalue( &t, "directory" );
		else if ( S_ISLNK( st.st_mode ) )
			lt_addtextvalue( &t, "link" );
		else if ( S_ISCHR( st.st_mode ) )
			lt_addtextvalue( &t, "character device" );
		else if ( S_ISBLK( st.st_mode ) )
			lt_addtextvalue( &t, "block device" );
		else if ( S_ISFIFO( st.st_mode ) )
			lt_addtextvalue( &t, "fifo" );
		else if ( S_ISSOCK( st.st_mode ) ) {
			lt_addtextvalue( &t, "socket" );
		}
		lt_finalize( &t );

		//get / save size
		lt_addtextkey( &t, "size" );
		lt_addintvalue( &t, st.st_size ); 
		lt_finalize( &t );

		//get permissions 
		#if 1
		char permissions[ 11 ] = "drwxrwxrwx\0";
		#else
		char permissions[ 11 ] = {0};
		permissions[ 0 ] = 'd';
		permissions[ 1 ] = 'r';
		permissions[ 2 ] = 'w';
		permissions[ 3 ] = 'x';
		permissions[ 4 ] = 'r';
		permissions[ 5 ] = 'w';
		permissions[ 6 ] = 'x';
		permissions[ 7 ] = 'r';
		permissions[ 8 ] = 'w';
		permissions[ 9 ] = 'x';
		#endif
		lt_addtextkey( &t, "permissions" );
		lt_addtextvalue( &t, permissions );
		lt_finalize( &t );

		//get owner
		//ownerfrom( st.st_uid );

		//get group
		//groupfrom( st.st_gid );
		
		//Get status changes, and access times
		char *aa = NULL;
		aa = ctime( &st.st_ctime );
		lt_addtextkey( &t, "date_changed" );
		lt_addtextvalue( &t, aa );
		lt_finalize( &t );

		aa = ctime( &st.st_atime );
		lt_addtextkey( &t, "date_accessed" );
		lt_addtextvalue( &t, aa );
		lt_finalize( &t );

		aa = ctime( &st.st_mtime );
		lt_addtextkey( &t, "date_modified" );
		lt_addtextvalue( &t, aa );
		lt_finalize( &t );

		//Come out
		lt_ascend( &t );
	}

	//Close the directory before moving forward
	if ( closedir( ds ) == -1 ) {
		lt_free( &t );
		snprintf( err, sizeof( err ), "Couldn't close directory %s", path );
		return http_set_error( res, 500, err ); 
	}

	//Do I need to do this?
	lt_ascend( &t );
	lt_lock( &t );
	zTable *tt = &t;

	//This may have helped more
	unsigned char *buf = NULL;
	int blen = 0;
	zRender *rz = zrender_init();
	zrender_set_default_dialect( rz );
	zrender_set_fetchdata( rz, &t );
	
	if ( !( buf = zrender_render( rz, templ, sizeof(templ), &blen ) ) ) {
		lt_free( &t );
		zrender_free( rz );
		return http_set_error( res, 500, "Render error" );
	}

	//Generate a message
	http_set_status( res, 200 );
	http_set_ctype( res, "text/html" );
	http_set_content( res, buf, blen );

	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		free( buf );
		lt_free( &t );
		//zrender_free( rz );
		return http_set_error( res, 500, err );
	}
	
	//Free everything
	lt_free( &t );
	zrender_free( rz );
	return 1;
}

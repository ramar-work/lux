#include "filter-dirent.h"
/*dirent.c - a directory handler*/
int filter_dirent ( struct HTTPBody *req, struct HTTPBody *res, void *p ) {
#if 0
	char path[PATHLEN] = {0};
	struct dirent *dir = NULL;
	struct stat st;
	char *dirname = NULL;
	DIR *ds = NULL;
	Table t;
	lt_init( &t, NULL, 1027 );

	if ( !( ds = opendir( dirname ) ) )
		return http_err( h, 404, "Page not found..." );

	//Get directory listing
	while ( (dir = readdir( ds )) ) 
	{
		//Check config to hide . & ..
		if ( dir->d_name && *dir->d_name == '.' )
			continue;	

		//Save pathname	
		snprintf(path, PATHLEN, "%s/%s", dirname, dir->d_name);

		//Configure what happens if the server can't access a file 
		if ( stat( path, &st ) == -1 )
			continue;

		//check file type
		lt_addblobkey( &t, "filetype", 8 ); 
		if ( S_ISREG( st.st_mode ) )
			lt_addblobvalue( &t, "file", 4 );
		else if ( S_ISDIR( st.st_mode ) )
			lt_addblobvalue( &t, "directory", 9);
		else if ( S_ISLNK( st.st_mode ) )
			lt_addblobvalue( &t, "link", 4);
		else if ( S_ISCHR( st.st_mode ) )
			lt_addblobvalue( &t, "character device", 16);
		else if ( S_ISBLK( st.st_mode ) )
			lt_addblobvalue( &t, "block device", 12);
		else if ( S_ISFIFO( st.st_mode ) )
			lt_addblobvalue( &t, "fifo", 4);
		else if ( S_ISSOCK( st.st_mode ) )
			lt_addblobvalue( &t, "socket", 6);

		lt_finalize( &t );

		//get / save size
		lt_addblobkey( &t, "size", 4 );
		lt_addintvalue( &t, st.st_size ); 
		lt_finalize( &t );

		//get permissions 
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
		lt_addblobkey( &t, (uint8_t *)"permissions", 11 );
		lt_addblobvalue( &t, (uint8_t *)permissions, 10 );
		lt_finalize( &t );

		//get owner
		//ownerfrom( st.st_uid );

		//get group
		//groupfrom( st.st_gid );
		
		//Get status changes, and access times
		char *aa = NULL;
		aa = ctime( &st.st_ctime );
		lt_addblobkey( &t, (uint8_t *)"date_changed", 11 );
		lt_addblobvalue (&t, aa, strlen( aa ));
		lt_finalize( &t );

		aa = ctime( &st.st_atime );
		lt_addblobkey( &t, (uint8_t *)"date_accessed", 11 );
		lt_addblobvalue (&t, aa, strlen( aa ));
		lt_finalize( &t );

		aa = ctime( &st.st_mtime );
		lt_addblobkey( &t, (uint8_t *)"date_modified", 11 );
		lt_addblobvalue (&t, aa, strlen( aa ));
		lt_finalize( &t );
		//Put it in a table
	}	

	//Render it
	//render_render( );

	//Free everything
	//render_free( &r );
#endif
	return 1;
}

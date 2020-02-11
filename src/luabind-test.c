#include "luabind.h"
#include "util.h"

#if 1
const char *string = "return {" \
"	-- Choose an engine" \
"	'template_engine' = 'mustache'," \
"" \
"	-- Choose a data source" \
"	'db' = {" \
"		main = 'copy'," \
"		banner = 'banner'" \
"	}," \
"" \
"	-- Logging function" \
"	'log' = function () do " \
"" \
"	end," \
"" \
"	-- Layout routes" \
"	'routes' = {" \
"" \
"		-- Standard MVC that loads files with the right name" \
"		default =    { " \
"			model = { 'log', 'default' }, " \
"			view = { 'main/head', 'default', 'main/footer' } " \
"		}," \
"" \
"		-- Do some PDF generation and put it in a folder" \
"		multi =      { " \
"			hint =   'Generates multiple PDFs for a student writing all PDFs to a folder and zipping it.'," \
"			model =  { 'log', 'check', 'view', 'multi' }, " \
"			view = { 'pdf/multi', 'pdf/confirmation-multi' } " \
"		}," \
"" \
"		-- Do some PDF generation and output a page " \
"		pdf =        { " \
"			hint =   'Generates a PDF for a student by aggregating all of the users who have requested student information.'," \
"			model =  { 'view', 'pdf' }, " \
"			view = { 'pdf/write', 'pdf/confirmation' } " \
"		}," \
"" \
"" \
"		-- Put a page behind basic auth" \
"		admin =      { " \
"			auth = 'basic'," \
"			model = { 'admin', 'list' }, " \
"			view = { 'main/head', 'admin', 'main/footer' } " \
"		}," \
"" \
"" \
"		-- Route multiple endpoints " \
"		'save|read|load' =       { " \
"			model = { 'middleware/date', 'log', 'check', 'pdf', 'save' }, " \
"			view = { 'main/head', 'save', 'main/footer' } " \
"		}," \
"" \
"	}" \
"}\n"; 
#endif


int main ( int argc, char *argv[] ) {
	//Create a Lua environment
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };
	int status = 0;

	//Load a string, and execute
	if ( !( status = lua_exec_string( L, string, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}

	//Load a file, and execute
	const char *file = "www/wrong.lua";
	if ( !( status = lua_exec_file( L, file, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}
	//status = lua_exec_string( L, string, err, sizeof(err) );

	//Load a file or string successfully...
	const char *file1 = "www/def.lua";
	if ( !( status = lua_exec_file( L, file1, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}

	//Dump the stack and show some stuff...
	lua_stackdump( L );

	//Load a bunch of files or strings and aggregate...
	const char *files[] = { "www/wrong.lua" };
	//Convert tables back and forth...

	return 0;	
}

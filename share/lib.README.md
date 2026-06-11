# lib

Shared objects, Lua files and other code that is shared by your application code can go here.   Installed C extensions end up here. Loading them from Lua within an application will look something like: 

```
local l = require( "lib.myextension" )

return {
	word = l.reverse( "Hello, world" )
}

```

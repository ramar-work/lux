# Documentation for Single

## Render

Setting up rendering with this library is a long ten-step process.     The first step is declaring a few types and initializing them.

<pre>
//Set up a table structure with space for at least 1024 keys
Table t;
lt_init( &t, NULL, 1024 );

//Set up a render structure
Render rn;
memset( &rn, 0, sizeof(Render));
render_init( &rn, &t );
</pre>

So what we've just done is set up a hash table, an instance of Table.  The rendering module uses this hash table when running its process.  The render_init step sets up some key fields in the Render structure.   It is written this way because the render buffer size will eventually be able to be set by the user.




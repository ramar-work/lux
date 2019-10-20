## hypno

Another attempt at writing some better server software in Lua of all languages...


## Build

- x OSX 
- x Linux


## Using the CLI

Hypno can create its own application directories like other big frameworks. 

It's usage is something like this:

Flags                 | Function
-----                 | --------
-c, --create [dir]    | Create a new application directory here.
-e, --eat [url]       | Feed this a certain URL and see how it evaluates.
-l, --list            | List all sites and their statuses.
-a, --at [dir]        | Create a new application directory here.
-n, --name [name]     | Use this as a site name.
-d, --domain [domain] | Use this domain.
-v, --verbose         | Be verbose.
-h, --help            | Show help and quit.

So, if I want to create a new site that can be served from anywhere, this
command would let that happen.

<pre>
$ ./hypno -c $HOME/mydir
</pre>



## Serving Something

Files in hypno are routes.  And do routey things.


## Caveats


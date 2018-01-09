## hypno

Another attempt at writing some better server software in Lua of all languages...


## Build

Builds on:
-   OSX 
- x Linux
-   Windows

Some work needs to be done here.


## Caveats

Many.


## Rationale

Why Build Another Server?
-------------------------
I built this because after months of trying to write a product using CF or node, I'm still having dumb problems and bad build processes. 

It was fairly challenging to create.  But the process of serving sites boils down to: 
- opening a socket
- parsing a message
- processing it via any means (could be raw HTML, scripting language, sending binary, database routine, etc)
- send the message back to the requestor
- close that process and listen again


Why use Lua?
------------
- It builds fine on OSX and Linux with a little bit of help
- It's dynamic.  So even though Go is a much better language on many levels, and C is more fun, it's easy to get changes in.
- Most union and set primitives are already written (with the exception of sort, extract and map) 
- Even though file system support is not included, the C library that hypno is built on top already includes this.
- Pretty easy to bind C to Lua.  
- Pretty easy to embed Lua into C, doing the same with PHP or Python requires a bit more legwork.
- The idea of a single unified data structure is appealing for beginners and I personally like the flexibility it offers.


Why not Javascript?
-------------------
- Javascript is a complicated language.
- For all of the things the language offers, it still manages not to ship with filesystem or database primitives.
- I have started to like Javascript less and less the longer I spend doing web development.
- Tooling, type safety and optimization all seem to be afterthoughts and only can be implemented through the use of third party libraries.  (There isn't even a real syntax checker outside of the browser.)
- Only one library exists that makes Javascript easy to bind to C (duktape - which is great!)


## How This Works

hypno is written from the ground up as an "all-in-one" website delivery tool.  It uses C for the hard and somewhat uninteresting stuff (like opening sockets, resource management, scripting language evaluation and HTTP parsing) and uses Lua for anything that could be classified as application layer material.  Configuration is done using simple files and tries to keep language idioms to an absolute minimum.

It (somewhat forcefully) enforces a model-view-controller paradigm for application design.  When new projects are created, the tool allocates disk space specifically for models, views and middleware.

All of hypno's sites rely on a file in the site root titled 'data.lua'.   Once the server receives a request for a particular domain name, the data.lua in the site root for that domain is parsed and evaluated.  If errors are present within the file, a 500 code will be sent back along with a pretty error message spelling out what actually happened.

Routing and site organization all happen from this file.  Here is an example:
<pre>
# hahaha, there is absolutely NOTHING here yet...
</pre>


## TODO

### Checklist

x Write, debug and test table aggregation

- Write, debug and test MVC execution chain
		Some problems here:
			rendering multiple levels deep does not work
			seeing the same issue that I saw months ago when looping, start index seems to be off
			also don't completely finish the render

- Write, debug and test data.lua evaluation

- Write, debug and test database drivers (SQLite should be built-in)

- Write, debug and test built-in routes (/dump, /debug, /info, whatever else) 

- Write sorts (certain methods may be wiser)

- Write string routines

- Write table extensions

- Write email primitives
	Hopefully a strong library already exists for this

- Write binary data generation routines (for testing, but instrumental because of how things should work)
	Writing against ImageMagick is the least resistance path for dynamic image generation
	GIF
	http://www.onicos.com/staff/iz/formats/gif.html
	http://giflib.sourceforge.net/whatsinagif/lzw_image_data.html

	However, I forgot that I'll probably want to write audio data (WAV, MP3 and FLAC)

	WAV
	https://gist.github.com/Jon-Schneider/8b7c53d27a7a13346a643dac9c19d34f
	http://www.topherlee.com/software/pcm-tut-wavformat.html
	http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
	https://sites.google.com/site/musicgapi/technical-documents/wav-file-format

	And common compression schemes (zip, lzma, gzip, etc)
	ZIP
	https://github.com/kuba--/zip
	https://code.google.com/archive/p/miniz/downloads
	https://zlib.net/ (hard to use possibly, hence why the two above exist, but its on most systems)
	http://7-zip.org/download.html (and here's another, which deals with .xz as well)

- Write encoding primitives ( base64, sha1sum, md5, etc )
	base64 is a search away

	the other two are probably covered in that big crypto book

- Write curl/wget/socket.get things and test APIs
	Paypal offers credit card numbers...
	https://www.paypalobjects.com/en_AU/vhelp/paypalmanager_help/credit_card_numbers.htm

### Aggregation

Debugged this, and now am ready to implement a solution.


### Test Suite

Most of these work fine, but:
tests/sql.c and tests/depth.c rely on shell scripts and installed user programs to generate test data.
Generating the data for these tests with C will solve speed and consistency problems.


### Packaging, Shipping and Maintenance

Linux is a mess.  So shipping Lua with the program might work best.   Single filing Lua is not going to be simple or plain.
That will need some kind of script.


### Stuff that's Leftover

Choosing Lua has been with many plusses and many negatives.  Some of the negatives mean:

- writing sort routines
- writing string manipulation functions
- writing some heavily needed table utilities
- write email primitives

To get this done, also requires:
- easy to build SSL
- JSON/XML parser 
- JS parser (maybe)


### Proposed CLI (Usage)

- handle a socket
	-s, -k = start, kill a server via hypno
	open it 
	receive data from it
	see what came parsed
	send data through it
	close it

- database drivers:
	-t [ mysql, sqlite3, postgresql, etc ] = test database drivers via hypno-test
	sqlite3
	mysql
	postgresql
	maybe one of the nosqls depending on need

- routing
	-u <file> = test a router setup via hypno (no json)
	interpret the routes in a file as they relate to code

- templating
	-f <file> = test templates with data and files, 
	-d {} or "select * from etc;" =  you'll need some sort of lua data 
	be able to do raw replacements
	be able to do scripting in the language (use the language)
	
- tooling
	this can be done from c too... no reason not to, but it can always be removed
	-c <dir> ...
	or
	--create-from <file> 
	shell script for now... is fine....
	can't move quickly w/o it
	create new directories and route structures
	remove directories
	possibly view what's in those directories

- handle the request / response chain
	obvs, there's nothing here...
	give the user some variables to choose from
	be able to set things that the user should be able to set
	after parsing, basic routes are always needed:
	/dump
	/debug
	/etc...

- run unit tests
	-t = run unit tests via hypno...	



## Tests

Right now all tests have to be run from the top-level directory.  There are two programs shipped that test certain parts of hypno.  `router` tests the router evaluation.  `chains` tests the MVC execution chain. `agg` tests table aggregation.  After compilation, each new program will show up in the top-level directory.

x agg
x render
x router
x sql

C chains


## Results

Thu, Jan  4, 2018  9:27:58 PM

spent about 2.5 hours getting back up to speed on hypno's codebase and working with a better test for aggregation.  working on a test program now that will randomly generate stack values.  The next step will be putting all of these values into one table. 


Mon, Jan  8, 2018 10:03:43 PM

Finished aggregation.  Run `agg` to see it work.

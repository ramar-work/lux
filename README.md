# hypno

A low maintenance web server and library for web development.


## Building

hypno depends on the following:

- Lua v5.3 or greater
- GnuTLS 2.0 or greater

Arch Linux is the distribution used for development, and as such it
is pretty easy to get it running there.  


### OSX

The OSX build currently relies on <a href="https://brew.sh">Brew</a> 
(which requires XCode command line tools and, in turn, XCode).  In the
future, this dependency will probably be removed.

<pre>

</pre>


### Windows (via Cygwin)


### Linux

Hypno is slowly moving over to GNU autoconf for a consistent build process.
Even in its current state, it still needs no exotic flags to build on
different platforms.

### Arch

<pre>
// Install the following dependencies
$ apt install liblua5.3-devel gnutls-dev

// Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

// Make and install
$ cd hypno && make && make install
</pre>



### Ubuntu / Debian / PopOS

<pre>
// Install the following dependencies
$ apt install liblua5.3-devel gnutls-dev

// Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

// Make and install
$ cd hypno && make && make install
</pre>


### Fedora

<pre>
# Install dependencies
$ yum install lua-devel gnutls-devel

# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples to see the code in action.
$ make examples
</pre>

### Arch


### BSD

<!-- ## Using the CLI -->
<!--  -->
<!-- Hypno can create its own application directories like other big frameworks.  It's usage is something like this: -->
<!--  -->
<!-- Flags                 | Function -->
<!-- -----                 | -------- -->
<!-- -c, --create [dir]    | Create a new application directory here. -->
<!-- -e, --eat [url]       | Feed this a certain URL and see how it evaluates. -->
<!-- -l, --list            | List all sites and their statuses. -->
<!-- -a, --at [dir]        | Create a new application directory here. -->
<!-- -n, --name [name]     | Use this as a site name. -->
<!-- -d, --domain [domain] | Use this domain. -->
<!-- -v, --verbose         | Be verbose. -->
<!-- -h, --help            | Show help and quit. -->
<!--  -->
<!-- So, if I want to create a new site that can be served from anywhere, this command would let that happen. -->


## Usage

This section is not finished yet.


<!-- ## Rationale -->
<!--  -->
<!-- Why Build Another Server? -->
<!-- ------------------------- -->
<!-- This was built as a cleaner answer to web frameworks written with Node.js, PHP, Lucee or other popular tools.  I wanted something simple to build which didn't require lots of dependencies.  -->
<!--  -->
<!-- The process of serving sites boils down to:  -->
<!-- - opening a socket -->
<!-- - parsing a message -->
<!-- - generating a response -->
<!-- - sending the message back to the requestor -->
<!-- - closing the socket -->
<!--  -->
<!-- Knowing this, I figured that there must be a better way to go about it than what we have today. -->
<!--  -->
<!--  -->
<!-- Why use Lua? -->
<!-- ------------ -->
<!-- - Lua builds fine on OSX and Linux. -->
<!-- - Packages and extensinons for Lua are available everywhere for a variety of operating systems. -->
<!-- - Lua is interpreted. -->
<!-- - Lua is fast. -->
<!-- - Most union and set primitives are already written (with the exception of sort, extract and map)  -->
<!-- - Pretty easy to bind C to Lua.   -->
<!-- - Pretty easy to embed Lua into C, doing the same with PHP or Python requires a bit more legwork. -->
<!-- - Lua relies heavily on the tables, a single unified data structure that is appealing to beginners and professionals alike. -->
<!--  -->
<!--  -->
<!-- Why not Javascript? -->
<!-- ------------------- -->
<!-- - Javascript has become overly complicated over the years. -->
<!-- - For all of the things the language offers, it still manages not to ship with filesystem or database primitives. -->
<!-- - I have started to like Javascript less and less the longer I spend doing web development. -->
<!-- - Tooling, type safety and optimization all seem to be afterthoughts and only can be implemented through the use of third party libraries.  (There isn't even a real syntax checker outside of the browser.) -->
<!-- - Only one library exists that makes Javascript easy to bind to C (duktape - which is great!) -->



## Notes

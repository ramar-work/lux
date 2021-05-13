# hypno

A low maintenance web server and library for web development.


## Building

hypno depends on the following:

- Lua v5.3 or greater
- GnuTLS 2.0 or greater


### Linux

Hypno is slowly moving over to GNU autoconf for a consistent build process.
Even in its current state, it still needs no exotic flags to build on
different platforms.


#### Debian 

Instructions for Debian ought to translate to other apt-based distributions.

<pre>
# Install the following dependencies
$ apt install liblua5.3-dev gnutls-dev make

# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples at port 2222
$ make examples
</pre>


#### Fedora

Instructions for Debian ought to translate to other yum-based distributions.

<pre>
# Install dependencies
$ yum install lua-devel gnutls-devel make

# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples to see the code in action.
$ make examples
</pre>


#### Arch & others

Arch happens to be the distribution that hypno is developed on and
it is a fairly straightforward process to build there.  
<pre>
# Install dependencies
$ pacman -Sy lua gnutls

# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples to see the code in action.
$ make examples
</pre>

SuSE, Slackware, Gentoo and others have not been tested yet, but the 
steps boil down roughly to the same thing.



### Windows (via Cygwin)

The Cygwin build is pretty close to what it would be on a typical
Linux system.  Dependencies can be grabbed either through the command-line
or through setup.exe.  That said, if you are not already using 
<a href="https://github.com/transcode-open/apt-cyg">apt-cyg</a>, 
you're missing out.

To install the dependencies via `apt-cyg`, try the following:
<pre>
$ apt-cyg install gnutls-devel lua-devel lua
</pre>

To install the via the setup.exe program, just search for the following
names: `gnutls-devel`, `lua-devel`, `lua`.

The program should find the newest versions and install them.

You will also need `gcc` and `make` if you do not typically build software
in your Cygwin environment. 

The rest of the build steps are follow what would be done a regular Linux
system.

<pre>
# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples at port 2222
$ make examples
</pre>

NOTE: In my opinion, running Hypno on Cygwin is really just
useful for quick tests.  The emulation and utilities used to get that
Linux feel just aren't fast enough for production-level performance.


### OSX

The OSX build needs a bit of work.   Building Lua manually and linking against
that seems to be the best solution for now. 

The OSX build also requires brew, which requires XCode and XCode's command line 
tools.

#### Getting Dependencies via Brew

The GnuTLS dependency can be grabbed via Brew.  Hypno will also require `pkg-config`
to find all of the libraries after installation.

<pre>
$ brew install gnutls pkg-config
</pre>

Unfortunately, the Lua package downloaded with Homebrew does not ship with 
it's headers or a library.  So we'll need to create one ourselves.

#### Getting and Building Lua

There are three steps to building Lua manually.

1. Visit http://www.lua.org/download.html
2. Download Lua 5.3.6 from http://www.lua.org/ftp/lua-5.3.6.tar.gz
3. Build the library and install it.

We can do all of this via the command line.
<pre>
$ curl -R -O http://www.lua.org/ftp/lua-5.3.6.tar.gz
$ tar xzf lua-5.3.6.tar.gz && cd lua-5.3.6
$ make macosx && make install
</pre>

#### Building Hypno

Building Hypno now will be simliar to other Linux based builds.

<pre>
# Get hypno via Git
$ git clone https://github.com/zaiah-dj/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples at port 2222
$ make examples
</pre>


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

### Server

Hypno currently ships with a server (hypno-server) capable of:

- Serving static web pages.
- Serving web applications via a shared object.
- Serving a list of files in a directory.
- Handling redirects.
- Serving TLS encrypted traffic.

The commands are listed below: 
<pre>
-s, --start                  Start the server              
-c, --config <arg>           Use this Lua file for configuration
-p, --port <arg>             Start using a different port  
-u, --user <arg>             Choose an alternate user to start as
-d, --dump                   Dump configuration            
    --no-fork                Do not fork                   
    --use-ssl                Use SSL                       
    --debug                  set debug rules               
-h, --help                   Show the help menu.  
</pre>


### Admin

Hypno also ships with a command line interface (hypno-cli) which will eventually
be capable of:

- Stress testing applications via the command line
- Modifying instances / applications
- Modifying domains within local DNS


### Configuration

Hypno leans on Lua's speed and simplicity for configuration tasks. 
See the example/ directory for what a typical server configuration
file looks like.


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



# hypno

A low maintenance library and tools for web development.


## Building from Source

hypno depends on the following:

- Lua v5.4 or greater
- GnuTLS 2.0 or greater

If cloning from Git, use the following command to create a configure script and Makefile:
```
autoupdate && autoreconf --install
```




### Linux

Hypno can be built from source like other common Linux software via the following steps.
<pre>
./configure && make && sudo make install
</pre>

Running `./configure --help` will display all of the different ways that the build can be customized for your system.   Your most important options are most likely going to be:

- `--with-server-user` - To define the user that owns the server process
- `--with-server-group` - To define the group that owns the server process


#### Debian 

Instructions for Debian will also be sufficient for other apt-based distributions.

<pre>
# Install the following dependencies
$ apt install liblua5.4-dev gnutls-dev make

# Get hypno via Git
$ git clone https://github.com/ramar-work/hypno.git

# Make and install (remember to use `sudo` when installing)
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
$ git clone https://github.com/ramar-work/hypno.git

# Make and install (remember to use `sudo` when installing)
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
$ git clone https://github.com/ramar-work/hypno.git

# Make and install
$ cd hypno && make && make install

# Run the examples to see the code in action.
$ make examples
</pre>



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
$ git clone https://github.com/ramar-work/hypno.git

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
$ git clone https://github.com/ramar-work/hypno.git

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


### App Development

Hypno's greatest use is as a general purpose web application server.  It can be extended to serve applications in a variety of languages, but first and foremost relies on Lua for the generation of what we'll call models.  For those familiar with the concept of MVC, a model is nothing more than a set of business logic.   Views by default are handled with a Mustache-esque (<a href="https://mustache.github.io">Mustache</a>) templating language.  

Hypno relies on three tools to work together in tandem: a server (hypno-server), a cli tool for administration (hypno-cli), and a testing engine (hypno-harness).   More information can be found #here, #here and #here. 

The folllowing command will create a new instance for your web application.

<pre>
$ hypno-cli -d /path/to/your/directory
</pre>

There are some additional options that will allow you to customize the domain name used, any static paths, and more.

If we `cd` into the newly created directory, we'll have a simple structure that looks something like this:

<pre>
app/
assets/
config.example.lua
config.lua
db/
favicon.ico
lib/
misc/
private/
ROBOTS.TXT
sql/
src/
views/
</pre>

Each one of the folders serves its own purpose, but that is slightly beyond the scope of this initial documentation.  See #here for more.


### Configuration

The file `config.lua` contains the application's title, fully qualified domain name, routes (manually specified) and a database connector depending on what kind of backend is in use (see #connectors for more info).  An example config file is below. 

<pre>
return {
	-- Database(s) in use 
	db = "sqlite3://db/roast.db",

	-- Title of our site
	title = "domo.fm",

	-- Fully qualified domain for our site
	fqdn = "domo.fm",

	-- Static assets
	static = { "favicon.ico", "ROBOTS.TXT", "/assets" },

	-- List of routes for our site
	routes = {
		-- home
		["/"] = { model="clients",view="index" },
		["login"] = { model="login",view="index" },
		["logout"] = { model="logout",view="index" },
		["dump"] = { model="dump",view="dump" },
		["add-user"] = { model="add-user",view="index" },
		["remove-user"] = { model="remove-user",view="index" },
		["profile"] = { 
			[":id"] = {
				model="user"
			, view="user" 
			}
		}
	}
}
</pre>

As you can see, our config file also specifies some default static paths that are needed for a majority of front-facing web applications.  These are `favicon.ico`, `ROBOTS.TXT` and anything under the `assets/` directory.  Editing the `/assets` path will, of course, modify which resources are accessible.

Notice the `db` key and value.   Hypno comes with both SQLite and MySQL drivers out of the box, and we can choose either by specifying the type of driver via URI and either a file or connection string depending on the desired backend.

Lastly, the config file expects a few keys by default, but we are free to add much more as needed.  For example, if we would like to keep track of a Google site verification ID, it can be done via something like the following:

<pre>
return {
	-- Database(s) in use 
	db = "sqlite3://db/roast.db",

	-- Let's add a Google verification key here.
	google_site_verification = "28349243-0234234-23423940",

	-- The rest of the keys
	...
}
</pre>

Since the config table is accessible to all Lua models, we can access this key from one of our models via `config.google_site_verification`.


### Routes 

Routes defined in `config.lua` follow a simple key value type of format.  For example, the following declaration:

<pre>
return {
	-- ....omitting the rest of the keys for brevity 
	routes = {
		["/"] = { 
			model = "home"
		, view = "home"
		}
	,	["namaste"] = {
			model = "peace"
		,	view = "peace"
		}
	}
}
</pre>

...will define two routes for this website.  One for root (/) and one for /namaste.   

hypno generates messages by executing the code in the files defined by the model key, and either returning that in a serialized format, or loading a view and sending the model output through that.  This means for our "/namaste" endpoint above, that hypno expects to find a file titled `$DIR/app/peace.lua` when looking for a model.  If hypno does not find it, it will return a 500 error, since the engine cannot find a required file.  Since we've also defined a view, hypno will expect to find a file titled `$DIR/views/peace.tpl`.   Errors in the template will return a 500, explaining what went wrong.  Likewise, a 500 will also be returned if the view file is not found.

The routes table also comes with some helpful conventions to make it easier to design large sites.  One convention is using a comma seperated list to let multiple routes point to the same set of files.  The following example will allow our site to serve requests for `/`, `/nirvana`, `/namaste`, `/enlightenment`.

<pre> 
return {
	-- ....omitting the rest of the keys for brevity 
	routes = {
		["/"] = { 
			model = "home"
		, view = "home"
		}
	,	["namaste,nirvana,enlightenment"] = {
			model = "peace"
		,	view = "peace"
		}
	}
}
</pre> 

Reusing the same example, with a slight modification, let's illustrate one way to serve different models.

<pre> 
return {
	-- ....omitting the rest of the keys for brevity 
	routes = {
		["/"] = { 
			model = "home"
		, view = "home"
		}
	,	["namaste,nirvana,enlightenment"] = {
			model = "@" -- What does this do?
		,	view = "peace"
		}
	}
}
</pre> 

Whenever the `@` symbol is specified in a model or view, hypno will search for a file matching the <i>active route</i>.  Using the the code above, if we get a request for `/namaste`, hypno now will expect a file named `$DIR/app/namaste.lua` to exist.  <i>One caveat to the `@` symbol evaluation, is that it does not work well with the home page requests.   This will change in a future version of hypno, but for right now, <b>just don't do it</b>.</i>

Lastly, the `model` and `view` do not always have to point to a string.   We can use both strings and tables and come up with some interesting combinations.  Let's say, for example, that we have a specific header that we want to load when serving requests for the home page.

<pre> 
return {
	-- ....omitting the rest of the keys for brevity 
	routes = {
		["/"] = { 
			model = "home" 
		, view = { "my-header", "home" }
		}
	,	["namaste,nirvana,enlightenment"] = {
			model = "@" -- What does this do?
		,	view = "peace"
		}
	}
}
</pre> 

Using the code above, our site will now load both `$DIR/views/my-header.tpl` and `$DIR/views/home.tpl` when getting a request forthe home page.   This can be extended even further with the `@` notation, allowing you to serve websites with a common template.  Making one last modification to our code above, we can modify the route table to load a header and footer when serving requests for `/namaste`, `/nirvana` and `/enlightenment`.

<pre> 
return {
	-- ....omitting the rest of the keys for brevity 
	routes = {
		["/"] = { 
			model = "home" 
		, view = "home"
		}
	,	["namaste,nirvana,enlightenment"] = {
			model = { "always-load-me", "@" }
		,	view = { "head", "@", "tail" }
		}
	}
}
</pre> 


### Models

Most any Lua code can be used when writing business logic, and packages can be installed via LuaRocks for extended functionality.  Additionally, Hypno comes with a large set of extensions that allow for a more cohesive experience when doing certain operations.  For example, the db module allows for fairly seamless (though simplistic one-time) connections to a few different dbms systems.   Sessions and common encoding/decoding routines also ship with Hypno, so unless there is a need for an encoding that does not exist, the engine will be able to serve most needs.

There are no real restrictions on models returned, but at the very least we'll need something like the following for a model to work:
<pre>
return {}
</pre>

A more realistic model will look more like this:
<pre>
-- Typical model
return {
	site_title = "My Fresh, Clean Website"
, site_author = "Juicy M. Johnson"
, site_rels = { 
		{ key = "Fresh" }
	,	{ key = "Clean" }
	,	{ key = "Spick & Span" }
	}
}
</pre>


### Views

Views are composed of simple mustache-like templates matching the keys that are returned from your models.  For example if we have code like this in a file called `example.lua`:

<pre>
return {
	number = 234234
, title = "List of Healthy and Unhealthy Foods"
, set = {
		{ calories = 15, food = "Seltzer" }
	,	{ calories = 1457, food = "Burger" }
	,	{ calories = 712, food = "Bowl of Pho" }
	}
}
</pre>

a template containing the following markup:
<pre>
&lt;html&gt;
&lt;h2&gt;{{ title }}&lt;/h2&gt;
&lt;ul&gt;
{{ #set }}
	&lt;li&gt;A "{{ .food }}" has {{ .calories }} calories.&lt;/li&gt;
{{ /set }}
&lt;/ul&gt;
Document #{{ number }}
&lt;/html&gt;
</pre>

will render something like this:
<pre>
<html>
<h2>List of Healthy and Unhealthy Foods</h2>
<ul>
	<li>A "Seltzer" has 15 calories</li>
	<li>A "Burger" has 1457 calories</li>
	<li>A "Bowl of Pho" has 712 calories</li>
</ul>
</html>
</pre>



## Tools 

Hypno comes with 3 different command line tools to help serve applications, which are discussed below.


### hypno-server

`hypno-server` handles: 

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


### hypno-harness

`hypno-harness` handles: 

- Testing of different routes from the command line (without a running server)

<pre>
hypno-harness:
-f, --filter <arg>       Specify a filter for testing (required).
-l, --library <arg>      Specify path to library.
-d, --directory <arg>    Specify path to web app directory (required).
-u, --uri <arg>          Specify a URI (required).
-c, --content-type <arg> Specify a content-type for testing.
-n, --host <arg>         Specify a hostname for use w/ the request.
-a, --alias <arg>        Specify an alternate hostname for use w/ the request.
-m, --method <arg>       Specify an HTTP method to be used when making
                         a request. (GET is default)
-p, --protocol <arg>     Specify alternate protocols (HTTP/1.0, 2.0, etc)
-F, --form <arg>         Specify a body to use when making requests.
-b, --binary <arg>       Specify a body to use when making requests. (assumes multipart)
                         (Use multiple invocations for additional arguments)
-e, --header <arg>       Specify a header to use when making requests.
                         (Use multiple invocations for additional arguments)
-M, --multipart          Use a multipart request when using POST or PUT
-S, --msg-only           Show only the message, no header info
-B, --body-to <arg>      Output body to file at <arg>
-H, --headers-to <arg>   Output headers to file at <arg>
-v, --verbose            Be wordy.
-h, --help               Show help and quit.
</pre>


### hypno-cli

`hypno-cli` handles: 

- Creating & modifying instances / applications

<pre>
No options received:
-d, --dir &lt;arg&gt;          Define where to create a new application.
-n, --domain-name &lt;arg&gt;  Define a specific domain for the app.
    --title &lt;arg&gt;        Define a &lt;title&gt; for the app.
-s, --static &lt;arg&gt;       Define a static path. (Use multiple -s's to
                         specify multiple paths).
-b, --database &lt;arg&gt;     Define a specific database connection.
-x, --dump-args          Dump passed arguments.
</pre>

<link rel="stylesheet" href="main.css">

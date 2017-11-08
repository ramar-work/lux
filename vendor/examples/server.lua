#!/usr/bin/env lua
------------------------------------------------------
-- @name
-- ------
-- server - Opens a listener on an unbound socket.
--
-- @synopsis 
-- ----------
-- None so far.
--
-- @description
-- -------------
-- Opens a listener on an unbound socket.  
--
-- @options
-- ---------
-- -p, --path <dir>      
-- 	Use <dir> as path to socket directory.
--
--     --root-path <dir>      
-- 	Use <dir> as path to handlers.
--
-- -l, --list 
--  	List the items that are in the sockets file and die.	
--
--     --lib <lib>       
-- 	Use <lib> as the shared object to load core files.
--
--     --dry-run 
-- 	Do a dry run instead of a actually serving requests.
--
-- -e, --user <user>     
-- 	Run the script as <user>
--
-- -m, --omit <list> 
-- 	Disallow the items in <list> from running.
--
-- -x, --diag            
-- 	Run in diagnostic mode (bypasses all handlers)
--
-- -o, --port <num>
-- 	Use an alternate port number.
--
-- -a, --address <ip-address>
-- 	IP address.
--
-- -x, --diag            
-- 	Run in diagnostic mode (bypasses all handlers)
--
-- -v, --verbose
-- 	Be verbose and tell every little thing that is happening.ca
--
-- -h, --help
--	Show help and quit.
--
-- @examples
-- ----------
-- None so far.
--
-- @caveats
-- ---------
-- Serving Files
-- =============
-- When starting a server, the path will point to a 
-- directory that has the user's images, files,
-- scripts and all that will contribute to actually
-- generating the application.  Hereafter, this will
-- be referred to as an instance.
--
-- The instance must always contain the following 
-- directories in order to start:
-- auth         - All authentication routines.
-- data         - Any local static data.
-- extensions   - Any extensions that don't ship with 
--                the library by default.
-- lib          - ?
-- log          - Logging files can go here, but it's
--                not required.
-- public       - All other world-readable static data
--                goes here.
-- skel         - All Lua scripts that are not an 
--                extension go here.
-- sockets      - All socket "descriptors" go here.
-- src          - Any source code used to build Lua
--                and C modules is placed here.
-- templates    - All templates for presentation go
--                here.
-- tmp          - A temporary directory that ships 
--                with Kirk (may be removed later)
-- 
-- Sockets 
-- =======
-- To start serving requests with a directory in mind,
-- edit a file called http.lua in the sockets directory.
--
-- The sockets directory has all the files needed to 
-- start whatever services you're planning on using.
-- They can be called whatever, but pay attention to the
-- handler field.  That will tell Kirk which protocol to
-- use when processing requests for the application. 
-- 
-- The variables and a short description of each are below:
-- port      = [0-65536],     	-- 80 is default
-- addr      = "127.0.0.1",   	-- IP Address 
-- socktype  = "tcp",	      	-- TCP is default
-- timeout   = 15,		-- Shouldn't take longer than this to serve?
-- die_after = 50,		-- Die after serving this many
-- backlog   = 10,		-- Allow 10 connections to wait.
-- handler   = "diag",		-- Use the 'diag' handler.
-- handler   = "http",		-- Use the 'diag' handler.
-- log       = "path",     	-- Most likely which type of log to keep (db or file) 
-- conc      = 10000,      	-- Handle 10000 concurrent connections.
-- read_at   = 1000,       	-- Read 1000 bytes per second
-- max_size  = 64000,      	-- Accept no payload larger than 64K
--
--
-- When receiving a request, Kirk will first look 
-- in the protocol folder.  
-- skel directory of the path supplied and if there
-- is nothing there, it will send the user to a default
-- 200 OK page. 
--
-- Structure of Documents
-- ======================
-- libkirk can serve as an application backend for
-- HTTP currently and will eventually be able to be
-- the backend for HTTPS, SMTP, RIP, SSH and many
-- other popular protocols.
--
--
-- SQL Database
-- ============
-- For now, libkirk includes SQLite as a datastore.
-- This will eventually be replaced by a tool called
-- Dragon, a tool written in Lua that serves as a 
-- native datastore.  It addresses the issues that
-- come with SQL in not being able to rebuild things
-- or needing seperate tools to recycle queries.
--
-- Technical Details
-- =================
-- The socket, log file (or database) are always open.
--
-- The socket's file descriptor is always queried to
-- see if the connection is still alive.  (this can
-- be changed to an interval if the service is not a
-- crucial one).
--
-- The following Lua search paths are used by default:
-- ./<file>.lua
-- /usr/share/lua/5.1/<file>.lua
-- /usr/share/lua/5.1/<file>/init.lua
-- /usr/lib/lua/5.1/<file>.lua
-- /usr/lib/lua/5.1/<file>/init.lua
-- ./<file>.dll
-- /usr/share/lua/5.1/<file>.dll
-- /usr/share/lua/5.1/loadall.dll
--
-- During heavy development, libraries will go under
-- bin/.
--
-- @return-values
-- --------------
-- 0 - Started and closed successfully.
-- 1 - Closed with SIGHUP or other abnormal message.
-- 2 - Too Many Connections
-- 3 - Directory for instance isn't valid. 
-- 4 - Socket didn't start correctly (use --verbose
--     or examine the error log (wherever the f**k it is)
--     for what went wrong.
-- 5 - Socket died prematurely.
-- 6 - Couldn't send data over socket.
-- 7 - Couldn't close socket.
--
-- @copyright
-- ----------
-- <mit>
--
-- @author
-- -------
-- <author>
--
-- @see-also
-- ---------
-- httpd(1)
--
-- @todo
-- -----
-- 	- What do you do if the env command is missing?
-- 	- Change this file to use named tables instead of numeric.
-- 	- Get rid of all these damned loops:
-- 		+ line 125 is fuckin ridiculous: come up with a better 
-- 		way to validate a directory.
-- 	- Other command line options
-- 	- C core file vs a generated Lua file.
--		- ipv6
--		- name resolution
-- 	- virtual host
--  	- memoization
--		- diagnostics
--		- tests
--		- update error messages and set it right 
--		- requests die on Cygwin
--
-- @end
-- ----
------------------------------------------------------

-- Include any dependencies
opt = require("opt")
loadall	= require("loadall")
-- render = require('render')

-- Make all errors 
local err = {}
PROGRAM = "server"
ERR_SUCC = 0
ERR_SIGHUP = 1
ERR_EXCCON = 2
ERR_DIRINV = 3
ERR_NOSTRT = 4
ERR_PREMXX = 5
ERR_NODATA = 6
ERR_NOCLSE = 7
err.succ  = 0
err.sighup = 1
err.exccon = 2
err.dirinv = 3
err.nostrt = 4
err.premxx = 5
err.nodata = 6
err.noclse = 7

------------------------------------------------------
-- perror(m, s)
--
-- Error out with a message and status.
------------------------------------------------------
function perror(m, s)
	print(PROGRAM .. ":", m)
	os.exit(s or 1)
end

------------------------------------------------------
-- usage (s)
--
-- Show a usage message and exit. 
------------------------------------------------------
function usage(s, m)
	if m then
		print (PROGRAM .. ":", m)
	end

	print("Usage: ./" .. PROGRAM)
	print([[
               [ ]

Startup options:
-p, --path <dir>        Use <dir> as startup path.
    --root-path <dir>   Use <dir> as handler path.
-l, --list              List the items that are in the sockets file.
-d, --dry-run           Do a dry run only.
-q, --debug             Show useful information.

Server options:
-o, --port <num>        Use <num> as the port to listen for requests.
-a, --address <ip>      Use <ip> as the IP address to bind to.
-s, --socket <type>     Change the socket type on the fly?
-t, --timeout <num>     Set a timeout value (in seconds)
-x, --die-after <num>   Die after <num> connections have been made.
-b, --backlog <num>     Only allow <num> connections to wait if using
                        a blocking model.
    --test              Run the test suite for this.
]])
	os.exit(s or 0)
end

------------------------------------------------------
-- Do any command line evaluations.
------------------------------------------------------
opt.config{
	clargs = {...},	-- Send command line options from here.
	name = PROGRAM, -- Setup this program's name.
	type = "string", -- Choose a type of option processor (in the works)
	to = "table"    -- A table to save results to
}

-- This is a better and cleaner solution.
-- opt{ validators = { ... }, options = {...}, config = { ... } } works too... 
-- opt.validators{ ... }

-- This is what currently works
args = opt.evaluate{
	-- Specify a socket path
	spath = {
		short = "-p",
		long  = "--path",
		exp   = 1,
	},
	
	-- Specify a handler root path.
	hpath = {
		short = "x",
		long  = "--root-path",
		exp   = 1,
	},

	-- Show help
	help = {
		short = "-h",
		long  = "--help",
		exp   = 0
	},

	-- Just show what's in pg
	list = {
		short = "-l",
		long  = "--list",
		exp   = 0
	},

	-- Do a dry run
	dry = {
		short = "-d",
		long  = "--dry-run",
		exp   = 0
	},

	-- Show debugging information.
	ddebug = {
		short = "-q",
		long  = "--debug",
		exp   = 0,
	},

	-- Log stuff.
	log = {
		short = "-o",
		long  = "--log",
		type  = "f",
		exp   = 1
	},

	-- Start on a different port.
	port = {
		short = "-o",
		long  = "--port",
		exp   = 1
	},

	-- Start on a different port.
	address = {
		short = "-a",
		long  = "--address",
		exp   = 1
	-- 	valid = <check if it's a tuple or ipv6, or possibly an fqdn>
	},

}

------------------------------------------------------
-- Shutdown if no arguments were received.
------------------------------------------------------
if table.maxn{...} < 1
then
	usage(1, "Needs at least the path before starting the server.")
end

------------------------------------------------------
-- If help was asked for, show it.
------------------------------------------------------
if args.help and args.help.set then usage(0) end

------------------------------------------------------
-- Include the core.
------------------------------------------------------
require("core")

------------------------------------------------------
-- Set all options
------------------------------------------------------
local server = {}
if args.port then server.port = args.port.value[1] end
if args.address then server.ip  = args.address.value[1] end 

------------------------------------------------------
-- Set any needed paths.
------------------------------------------------------
local conn, g, ipath    = {}, {}, {}, {}
local path = {
	-- Can be anything with socket files 
	main = args.spath.value[1],   			

	-- Can be anything with socket files 
	skel = args.spath.value[1] .. "/skel",  

	-- Typically $HOME/path-to-libkirk-dir/
	root = args.hpath.value[1],

	-- These will probably not exist in the future.
	inc  = args.hpath.value[1]  .. "/include",
	hand = args.hpath.value[1]  .. "/include/handlers",
	util = args.hpath.value[1]  .. "/include/util"
}

------------------------------------------------------
-- Check for all socket files, load them and start
-- them.  If for some reason, one of the files should
-- not be started, it can be commented out and omitted
-- in that fashion.  Or use the --omit flag from the
-- command line.
------------------------------------------------------
-- Loop through all needed folders and check that each of these exist.
for _,nm in ipairs{
	"auth",		-- Authentication information.
	"data",		-- Database information.
	"log",		-- Logging information.
	"public", 	-- Public directory for serving raw mime, etc.
	"skel", 	-- Templates for logic.
	"templates", 	-- Templates for presentation.
--	"tmp", 		-- Temporary write space, can change.
} 
do
	if not files.exists(path.main .. "/" .. nm).status
	then
		perror("Folder "..nm.." does not exist.")
	end
end

------------------------------------------------------
-- I don't know why this is here...
------------------------------------------------------
for k,v in pairs(files.dir(path.main).results)
do
	if v.filetype == "directory" 
	then
		ipath[v.basename] = v.realpath
	end
end

------------------------------------------------------
-- Setting all the utilities to be global.
------------------------------------------------------
for k,v in pairs((loadall(path.util)).results)
do
	if not _G[k] 
	then 
		_G[k] = v
	else
		-- Set any already global keys
		for kk,vv in pairs(v) do _G[k][kk] = vv end
	end
end


------------------------------------------------------
-- Start the server loop for each socket file.
------------------------------------------------------
f	= files.dir(path.main .. "/sockets")
if f.status
then
	-- Loop through all the files within the sockets directory.
	for k,v in ipairs(f.results)
	do
		-- 1. For the sake of speed, and because I hate writing 
		-- if v.basename ~= ".." and k ~= "." and k ~= "status"
		local r = dofile(v.realpath)
		local x = string.gsub(v.basename, ".lua", "")

		-- Start a socket.
		conn[x] = socket.start(
			r.port or 80,           -- Default listening port. table for multiple 
			r.addr or "127.0.0.1",	-- Defualt host.
			r.socket or "tcp",	   -- Use a TCP socket (UDP and RAW also work) 
			r.timeout or 5,  	      -- Default connection timeout time.
			r.die_after or 3,  	   -- Default shutdown parameter.
			r.backlog or 10		   -- Allow this many connections at once. 
		)	

		-- Mate this open socket to the parent process.
		parent = conn[x]
		parent.count = 0

		if not parent.status
		then
			print("Could not start socket connection.")
			print(parent.errv)
			os.exit()
		end


		-- Load a handler.
		loadpath = path.hand .. "/" .. (r.handler or "diag")
		handler = loadall(loadpath, {
			__rule = { lua = dofile }, -- txt = fread, tpl = fread },
			__env	 = { private = private, public = public },
			__path = "../handlers/echo",
			-- __exc  = { "a", "b", "c", "d/x" }
			-- __rec  = 2,3,4,n  -- How deep should this go?
		})

		-- Debugging verbose details...
		if args.ddebug and args.ddebug.set 
		then
			print(table.concat({
				"Server starting on:",
				"-------------------",
			        "",	
				"Port:     " .. r.port,
				"Addr:     " .. r.addr,
				"Timeout:  " .. r.timeout,
				"Shutdown: " .. r.die_after,
				"Backlog:  " .. r.backlog,
				"Handler path: " .. loadpath,
				"Socket path:  " .. path.main,
			        "",	
				"Awaiting requests...",
				"--------------------"
			}, "\n"))

			-- No good way to debug this right now...
			for k,v in pairs(handler.results) 
			do 
				print(k,v) 
				if k == "util" then
					for x,y in pairs(v) 
					do
						print(x,y) 
					end	
				end	
			end	
		end

		if handler.status 
		then
			-- A list of the possible handlers goes here.
			handler = handler.results

			while ( parent.count < r.die_after  )
			do
				child = socket.accept( parent.fd )
				if child.status
				then
					-- Read the data from the socket.
					read = socket.read( child.fd, 1, 64000, 240, "\r\n\r\n" )
					if not read.status
					then
						print(read.errv)
						os.exit()
					end

					-- Check for any errors here too.
					parse = handler.parse(read.results)
					-- for k,v in pairs(parse) do print(k,v) end
		
					-- Send the payload over the wire.	
					-- loadall does NOT need to run again...
					-- if the response is a string, packaging it as a function
					-- will delay the response....
					send = socket.send(
						child.fd, 
						handler.main(path.skel, parse, true)
					)
					if not send.status
					then
						-- Let's do the rest of the sending here if it fails.
						print(send.errv)
						os.exit()
					end
			
					-- Close the socket down.	
					close = socket.stop( child.fd )
					if not close.status
					then
						print(close.errv)
						os.exit()
					end
				end

			-- Parent count 
			parent.count = parent.count + 1
			-- print(r.die_after, parent.count)
			end

		-- Stop the socket.
		socket.stop(parent.fd)
		end
	end -- a.status
end

--[[-------------------------------------------------
tests:
get: GET /x/y/z/y/b HTTP/1.1

get:
	A : B	
	(A:B) : C

post:
put:
--]]------------------------------------------------

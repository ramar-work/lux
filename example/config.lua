-------------------------------------------------
-- config.lua
-- ==========
--
-- @summary
-- Server config file for example htdocs of libhypno. Running the target 
-- `make examples` will start a server with these four sites loaded.  
--
-- NOTE: The names below will need to be listed in your /etc/hosts file to
-- resolve on your system.
--
-- @usage
-- Sites can be added by simply creating the directory you want, and 
-- adding it to the list below.
--
-- @changelog
-- nothing yet...
-- 
-------------------------------------------------
return {
	wwwroot = "example",
	hosts = {
		-- Default host in case no domain is specified
		["localhost"] = { 
			root_default = "/index.html",
			dir = "localhost",
			filter = "static"
		},
	
	--[[	
		-- Tests (these can be built into the engine)
		["cfilter.local"] = { filter = "c" },
		["echofilter.local"] = { filter = "echo" },
		["staticfilter.local"] = { 
			filter = "static", 
			alias = "static.local",
			dir = "static.local",
			root_default = "/index.html" 
		},
	--]]

		-- Redirect does nothing of real importance
		["redirect.hypno.local"] = { 
			-- Where to redirect?
			filter = "redirect"
		},

		-- See the dirent filter in action 
		["dir.hypno.local"] = { 
			dir = "dir",
			root_default = "/index.html",
			filter = "dirent"
		},

		-- See the echo filter in action
		["echo.hypno.local"] = { 
			root_default = "/index.html", -- Technically, no root is needed
			filter = "echo"
		},

		-- See the static filter in action
		["html.hypno.local"] = { 
			dir = "html",
			root_default = "/index.html",
			filter = "static"
		},
	
		-- See the static filter in action
		["file.hypno.local"] = { 
			dir = "file",
			root_default = "/index.html",
			filter = "static"
		},

		-- A Lua directory...
		["lua.hypno.local"] = { 
			dir = "lua",
			filter = "lua"
		},

		-- Big file tests
		["ironheadrecordings.local"] = { 
			dir = "file",
			root_default = "/index.html",
			ca_bundle = "misc/self-signed/ironhead_self_signed.ca-bundle",
			cert_file = "misc/self-signed/ironhead_self_signed.crt",
			key_file = "misc/self-signed/ironhead_self_signed.key",
			filter = "static"
		},

		-- self-signed certificate will match with ironheadrecordings.local
		-- ["ironheadrecordings.local"] = { 
		["tls.hypno.local"] = { 
			dir = "tls",
			-- root_default = "/index.html",
			ca_bundle = "misc/tls/self-signed/ironhead_self_signed.ca-bundle",
			cert_file = "misc/tls/self-signed/ironhead_self_signed.crt",
			key_file = "misc/tls/self-signed/ironhead_self_signed.key",
			filter = "lua"
		},

		-- supergreatwok.xyz
		["supergreatwok.xyz"] = { 
			dir = "tls",
			-- root_default = "/index.html",
			ca_bundle = "misc/tls/supergreatwok.xyz/supergreatwok_xyz.ca-bundle",
			cert_file = "misc/tls/supergreatwok.xyz/supergreatwok_xyz.crt",
			key_file = "misc/tls/supergreatwok.xyz/supergreatwok_xyz.pem",
			filter = "lua"
		},
	}
}

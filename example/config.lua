-------------------------------------------------
-- config.lua
-- ==========
--
-- Server config file for example htdocs of libhypno. Running the target 
-- `make examples` will start a server with these four sites loaded.  
--
-- NOTE: The names below will need to be listed in your /etc/hosts file to
-- resolve on your system.
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
		-- A Lua site
		["localhost"] = { 
			root_default = "/index.html",
			dir = "localhost",
			filter = "static"
		},

		-- self-signed certificate will match with ironheadrecordings.local
		["tls.hypno.local"] = { 
			dir = "tls",
			-- root_default = "/index.html",
			ca_bundle = "misc/tls/self-signed/ironhead_self_signed.ca-bundle",
			cert_file = "misc/tls/self-signed/ironhead_self_signed.crt",
			key_file = "misc/tls/self-signed/ironhead_self_signed.key",
			filter = "lua"
		},
		--]]
	}
}

-------------------------------------------------
-- config.lua
--
-- Configuration file for demos only.
--
-------------------------------------------------
return {

	-- The webroot for all sites
	wwwroot = "/home/ramar/prj/lux-2025/lb1/test/www",

	-- The list of hosts that should be picked up.	
	hosts = {

		-- Demo of a Lua backend strictly for JSON messaging
		["json.demo.local"] = { dir = "json_demo", filter = "lua", alias = "json.local" },

		-- Demo of Lua backend
		["lua.demo.local"] = { dir = "lua_demo", filter = "lua", alias = "lua.local" },

		-- Package demo
		-- TODO: This doesn't currently work
		-- ["package.demo.local"] = { dir = "package_demo", filter = "lua", alias = "package.local" },

		-- Simple, insecure HTTP webpages
		["static.demo.local"] = { dir = "static_demo", filter = "static", alias = "static.local", root_default = "/index.html" },

		-- Self signed TLS demo.  Same process for other TLS backend sites. 
		-- ["tls.demo.local"] = { dir = "tls_demo", filter = "static", alias = "tls.demo.local", cert_file = "", key_file = "" } -- , ca_bundle = "" },

		-- TODO: Add a C/C++ filter just to show how it's done
		--["tls.demo.local"] = { dir = "tls_demo", filter = "c", alias = "tls.demo.local" },
	}
}

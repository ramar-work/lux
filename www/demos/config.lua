-------------------------------------------------
-- config.lua
--
-- Configuration file for demos only.
--
-------------------------------------------------
return {

	-- The webroot for all sites
	wwwroot = ".",

	-- The list of hosts that should be picked up.	
	hosts = {

		-- Demo of a Lua backend strictly for JSON messaging
		["json.demo.local"] = { dir = "json_demo", filter = "lua", alias = "json.local" },

		-- Demo of Lua backend
		["lua.demo.local"] = { dir = "lua_demo", filter = "lua", alias = "lua.local" },

		-- Simple, insecure HTTP webpages
		["static.demo.local"] = { dir = "static_demo", filter = "static", alias = "static.local", root_default = "/index.html" },

	}
}

-- Config for just hypno, should I serve it on its own...
return {
--port = 2000,
	wwwroot = "./docs",
	hosts = {
		-- Hypno Repo
		["hypno.local"] = { 
			dir = "static",
			alias = "hypno.ironhead.local",
			filter = "static",
			root_default = "/index.html"
		},

		["ramar.local"] = {
			dir = "fake",
			filter = "lua"
		}
	}
}

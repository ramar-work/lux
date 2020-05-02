return {
	hosts = {
		-- A static host with regularly sized files
		["localhost"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},

		-- Test SSL
		["tls.local"] = { 
			dir = "tests/filter-static/tls",
			root_default = "/index.html",
			ca_bundle = "",
			cert_file = "",
			keyfile = "",
			filter = "static"
		},

		-- Test large files 
		["large.local"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},
	}
}

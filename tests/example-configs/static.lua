return {
	hosts = {
		-- A static host with regularly sized files
		["localhost"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},

		-- Test SSL
		["hypnotls.local"] = { 
			dir = "tests/filter-static/tls",
			root_default = "/index.html",
			ca_bundle = "misc/x509-ca.pem",
			certfile = "misc/x509-server.pem",
			keyfile = "misc/x509-server-key.pem",
			filter = "static"
		},

		-- Test large files 
		["hypnobig.local"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},
	}
}

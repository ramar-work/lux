return {
	number = 777,
	wash = 32423, 
	hosts = {
		-- A static host
		["localhost"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},

		-- A static host (I'm GUESSING, that we're crashing because no host points anywhere)
		["collinshosting.com"] = { 
			dir = "tests/filter-static/text",
			root_default = "/index.html",
			filter = "static"
		},

		-- Charlotte Software Developer (learn how to do stuff)
		["csdev.local"] = { 
			dir = "/home/ramar/prj/charlotte-software-dev/www-hypno",
			root_default = "/index.html",
			filter = "static"
		},

		-- Eventually the home of my portfolio
		["collinsdesign.net"] = {
			alias = "collinsdesign.local",
			dir = "/home/ramar/prj/collinsdesign.local",
			filter = "static"
		},

		-- Perhaps use this for SSL tests
		["porsche.local"] = { 
			alias = "porsche.lk",
			ca_cert = "",
			keyfile = "",
			root_default = "/index.html",
			dir = "tests/filter-static/text",
			filter = "static"
		}
	}
}

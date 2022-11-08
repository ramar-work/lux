-- B
return {
	number = 777,
	wash = 32423, 
	hosts = {
		["localhost"] = { 
			dir = "www",
			filter = "lua"
		},

		["collinsdesign.net"] = {
			alias = "collinsdesign.local",
			dir = "/home/ramar/prj/collinsdesign.local",
			filter = "static"
		}
	}
}

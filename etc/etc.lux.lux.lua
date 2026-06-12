-- 
return {
	
	-- Repository thing 
	repository = "file:///home/ramar/repo",

	-- Log
	log = {
		access = "file:///home/ramar/access.log",
		error = "file:///home/ramar/error.log",
		style = "combined"
	},

	-- User
	user = "ramar",

	-- Group
	group = "ramar",

	-- PID
	pid = "",

	-- Port
	port = 2222,

	-- Protocols
	protocols = {
		{ type = "http", port = 80 }
	}
}

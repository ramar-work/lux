return {
	db = "none",
	title = "json_demo",
	fqdn = "api.test",
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },
	routes = {
		["/"] = { model="hello" },
		books = {
			model = "fetch",
			[":id=number"] = { model="fetch" },
		},
	}	
}

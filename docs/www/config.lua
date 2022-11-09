return {
	db = "none",
	title = "www",
	fqdn = "www",
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },
	routes = {
		["/"] = { model="hello",view="hello" },
		stub = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}

return {
	db = "none",
	title = "lua.local",
	fqdn = "lua.local",
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },
	routes = {
		["/"] = { model="hello",view="hello" },
		stub = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}

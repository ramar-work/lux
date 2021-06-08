return {
	db = @db@,
	title = @title@,
	fqdn = @fqdn@,
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },
	routes = {
		["/"] = { model="hello",view="hello" },
		stub = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}

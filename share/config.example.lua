return {
	-- Can specify sqlite, or mysql
	db = "roast.db",

	-- Define <title>, can only be a string
	title = "da food snob",

	-- Define fully qualifed domain name, can only be a string
	fqdn = "dafoodsnob.com",

	-- Define a template engine, if not mustache
	-- template_engine = "ejs",

	-- Define static paths (usually a table)
	static = {
		"/ROBOTS.TXT"
	, "/assets"
	}

	-- Define routes that you'll manage
	routes = {
		default = { model="roast",view="roast" },
		turkey = { model="turkey",view="roast" },
		chicken = { model="chicken",view="roast" },
		beef = { model="beef",view="roast" },
		recipe = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}

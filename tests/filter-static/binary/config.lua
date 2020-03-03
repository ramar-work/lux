return {
	db = "roast.db",
	title = "da food snob",
	fqdn = "dafoodsnob.com",
	template_engine = "roast.db",
	static = "static",
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

return {
	-- Database(s) in use 
	db = "misc/roast.db",

	-- Title of our site
	title = "domo.fm",

	-- Fully qualified domain for our site
	fqdn = "domo.fm",

	-- Default template engine for our site 
	--template_engine = "roast.db",

	-- List of routes for our site
	routes = {
		-- home
		["/"] = { model="home",view={ "head", "body", "tail" } },

		-- user management
		login = { model="index",view={ "head", "login", "tail" } },
		register = { model="index",view={ "head", "register", "tail" } },
		logout = { model="index",view={ "head", "logout", "tail" } },

		-- :id 
		artist = { model="artist",view={ "head", "artist", "tail" } },
		label = { model="label",view={ "head", "label", "tail" } },
		track = { model="track",view={ "head", "track", "tail" } },

		-- static pages
		faq = { model="faq",view={ "head", "faq", "tail" } },
		app = { model="app",view={ "head", "app", "tail" } },

		-- info
		stats = { model="stats",view={ "head", "stats", "tail" } },

	--[[
		-- store
		store = { model="store",view={ "head", "store", "tail" } },

		-- individual profile data (managed by users)
		profile = { model="profile",view={ "head", "profile", "tail" } },

		turkey = { model="turkey",view="roast" },
		chicken = { model="chicken",view="roast" },
		beef = { model="beef",view="roast" },
		recipe = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	--]]
	}
}

--[[ -------------------------------------------------- 
-- data.lua
--
-- ## Summary
-- A better way to go about making a dynamic website.
--
-- ## Tidbits and Details
-- This actually has a lot of serious benefits.  In 
-- particular, the fact that I can execute code here
-- that will translate to whatever I'm trying to run.
-- 
-- Also, this really negates the need for routes in
-- files.  But that's an important detail, so that will
-- probably stay.
-- 
-- ## Folder structure
-- Apps written this way work kind of like the bottom
-- diagram here:
--
-- app/           - Model/business logic files go here.
-- assets/        - CSS, SASS, etc here
-- components/    - Things that should stay in memory here*
-- db/            - Databases go here (file-based)
-- files/         - Written files can go here, but there are choices
-- log/           - Logs are written here or somewhere else
-- middleware/    - Files that are just reused go here
-- routes/        - Routes that were supposed to be added went here, but 
--                  (I think this is going away)
-- setup/         - Setup scripts?
-- sql/           - SQL files or query files can go here
-- std/           - (These would be built-in templates...)
--                  (I think this is going away)
-- views/         - This is where templates go
-- data.lua       - Everything is <i>organized</i> from here.
-- index.lua      - I'd have no trouble using this instead
--                  (I think this may show up)
-- ---------------------------------------------------- ]]
return {
	source =     "appCopy",
	cookie =     "02349203kjsadfksadf",
	base =       "/ferpa/",
	Home =       "tmp",
	name =       "Ferpa",
	title =      "FERPA (Family Educational Rights and Privacy Act )",
	debug =      0,
	masterPost = false,
	settings =   {
		verboseLog = 0,
		addLogLine = 0
	},
	db = {
		main =   "appCopy",
		banner = "banner-prod"
	},
	css =        {
		"zero", 
		"gallery", 
		"https://unpkg.com/purecss@0.6.2/build/pure-min"
	},
	js =         {
		"lib", 
		"index" 
	},
	routes =     {
		default =    { 
			model = { "log", "default" }, 
			view = { "main/head", "default", "main/footer" } },

		-- Choosy pigeon
		multi =      { 
			--hint =   "Generates multiple PDFs for a student writing all PDFs to a folder and zipping it.",
			model =  "simple",
			view = "simple" 
		},

		realBig =      { 
			hint =   "Generates multiple PDFs for a student writing all PDFs to a folder and zipping it.",
			model =  { "log", "check", "view", "multi" }, 
			view = { "pdf/multi", "pdf/confirmation-multi" } },

		pdf =        { 
			hint =   "Generates a PDF for a student by aggregating all of the users who have requested student information.",
			model =  { "view", "pdf" }, 
			view = { "pdf/write", "pdf/confirmation" } },

		save =       { 
			model = { "middleware/date", "log", "check", "pdf", "save" }, 
			view = { "main/head", "save", "main/footer" } 
		},

		intro =      {
			model = "intro", view = { "main/head", "intro", "main/footer" } },

		admin =      { 
			model = { "admin", "list" }, 
			view = { "main/head", "admin", "main/footer" } },
	}
}



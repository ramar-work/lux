-------------------------------------------------
-- config.lua
-- ==========
--
-- An example instance configuration file.  Just
-- about anything can be specified here, but the
-- following keys are the only things that are
-- used by Lua directly:
--
--
-- key    | type   | description
-- ---    | ----   | -----------
-- db     | string | A primary database file or connection.
-- title  | string | A site title, used for HTML files
-- fqdn   | string | A fully qualified domain name
-- static | table  | A table containing a list of static paths.
-- author | string | A default author name
-- routes | table  | A table containing the endpoints of the site
-- pre    | funct  | A function which will run before models are evaluated
-- post   | funct  | A function that will run after models are evaluated
-- ctype  | string | The default content-type string
--
--
-- Referencing elements within this config file can be done
-- from both model (everything under `app/`) and template
-- files (everything under `views/`).  Just call `config.$ELEMENT`
-- for the key you want.
--
--
-- Example:  We want config.title in a template.
-- <pre>
-- <html>
-- <head>
-- 	<title>{{ config.title }}</title>
-- </head>
-- </html>
--
--
-------------------------------------------------
return {
	-- A default database connection
	db = "@db@",

	-- Site title
	title = "@title@",

	-- Fully qualified domain
	fqdn = "@fqdn@",

	-- Define a default author
	-- author = "@author@",

	-- Define a custom logger
	-- logger = "@x@",

	-- Default pre runner (if any)
	-- pre = function() end

	-- Default post runner (if any)
	-- post = function() end

	-- Default content type
	-- ctype = "@x@",

	-- List of static paths
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },

	-- List of routes
	-- (Additional tables in the routes/ directory will be added to this table.)
	routes = {

		-- All sites have a default route at '/'
		["/"] = { model="hello",view="hello" },

		-- `stub` defines a route that can be referenced with `/stub`
		stub = {
			-- `id=number` is a parameter under `/stub`, accessed like `/stub/21`
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}

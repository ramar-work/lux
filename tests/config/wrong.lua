	--[[
		-- Example
		$routeName = {
			auth = " basic | query = ... | ... ",
			method[s] = "get | * | post, get",
			fail = "...",
			succeed = "...",	
			expects = { list of values }
			model[s] = { ... },
			view[s] = { ... }
			hint[s] = "what does this do?"
			query = "...."
			contentType = "json | ... | etc"
		}
	]]


return {
	-- Choose an engine
	"template_engine" = "mustache",

	-- Choose a data source
	"db" = {
		main = "copy",
		banner = "banner"
	},

	-- Logging function
	"log" = function () do 

	end,

	-- Layout routes
	"routes" = {

		-- Standard MVC that loads files with the right name
		default =    { 
			model = { "log", "default" }, 
			view = { "main/head", "default", "main/footer" } 
		},

		-- Do some PDF generation and put it in a folder
		multi =      { 
			hint =   "Generates multiple PDFs for a student writing all PDFs to a folder and zipping it.",
			model =  { "log", "check", "view", "multi" }, 
			view = { "pdf/multi", "pdf/confirmation-multi" } 
		},

		-- Do some PDF generation and output a page 
		pdf =        { 
			hint =   "Generates a PDF for a student by aggregating all of the users who have requested student information.",
			model =  { "view", "pdf" }, 
			view = { "pdf/write", "pdf/confirmation" } 
		},


		-- Put a page behind basic auth
		admin =      { 
			auth = "basic",
			model = { "admin", "list" }, 
			view = { "main/head", "admin", "main/footer" } 
		},


		-- Route multiple endpoints 
		"save|read|load" =       { 
			model = { "middleware/date", "log", "check", "pdf", "save" }, 
			view = { "main/head", "save", "main/footer" } 
		},

	}
}

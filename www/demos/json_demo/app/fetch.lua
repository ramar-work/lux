-------------------------------------------------
-- fetch.lua
--
-- A list of books for this test demo.
--
-------------------------------------------------
local booklist = {
{
	id = 0,
	author = "Orson Scott Card",
	title = "Ender's Game",
	year = 1950,
	isbn = "978-0812550702" 
},

{
	id = 1,
	author = "Alice Walker",
	title = "The Color Purple",
	year = 1982,
	isbn = "978-0156028356" 
},

{
	id = 2,
	author = "Paulo Coelho",
	title = "The Alchemist",
	year = 1993,
	isbn = "978-0061122415",
},

{
	id = 3,
	author = "Isaac Asimov",
	title = "I, Robot",
	year = 1950,
	isbn = "978-0553382563",
},

{
	id = 4,
	author = "Danielle Steel",
	title = "Zoya",
	year = 1987,
	isbn = "978-0385296496" 
},

}

-- Serve root requests
if route.active == "books"
then
	return booklist

--[[
-- Add search capability
--]]

-- Find the right book
else
	for i,x in ipairs( booklist ) do
		if x.id == tonumber(route.active)
		then
			return x
		end
	end	
	
	-- This is an instant 404 otherwise.
	-- TODO: response.send( 404, { error: "Entry not found." } )
	response = {
		status = 404,
		headers = { ["Content-Type"] = "application/json" },
		content = json.encode{ error = "Entry not found." }
	}
end




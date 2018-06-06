--[[ -------------------------------------------------- 
-- #  data.lua
-- 
-- ## Summary
-- Routes and other info needed to serve a site.
-- 
-- ## Architecture
-- data.lua       - This file, where everything is organized
-- assets/        - CSS, SASS, etc here
-- db/            - Databases go here (file-based)
-- files/         - Written files can go here, but there are choices
-- models/        - Model/business logic files go here.
-- sql/           - SQL files or query files can go here
-- views/         - This is where templates go
-- *log/          - Logs may be written here or somewhere else
--
-- ## Project
-- This site serves as the backend for a user's Savings Goals.
-- It can also be used for other goals, but the tools here lend
-- to reaching a financial goal of some sort.
-- ---------------------------------------------------- ]]
return {
	routes =     {
		-- The default page, I'll just get the user's goals here
		default =    { 
			model = "default", 
			view = "default"
		},
	}
}

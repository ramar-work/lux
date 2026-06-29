-- db.lua
-- Test all database engines with and without bind arguments
local sqlite_db = "sqlite3://db/test.db"
db.exec{
	conn = sqlite_db
, string = "SELECT * FROM profiles"
}



--[[
local mysql_db = "sqlite3://db/test.db"
db.exec{
	conn = sqlite_db
, string = "SELECT * FROM profiles"
}
--]]

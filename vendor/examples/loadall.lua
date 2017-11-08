#!/usr/bin/env lua
------------------------------------------------------
-- @name
-- ------
-- loadall - Loads all of a type of direcotry
--
-- @synopsis 
-- ----------
-- loadall( dir [string], exclude [table] )
--
-- @description
-- -------------
-- Loads all of a particular type of file in a directory
-- according to preset rules.
--
-- @options
-- ---------
-- ?
--
-- @examples
-- ----------
--
-- @caveats
-- ---------
--
-- @copyright
-- ----------
-- <mit>
--
-- @author
-- -------
-- <author>
--
-- @todo
-- -----
--	- Build it into C to keep module maintenance at a minimum
-- 	- To write this in C involves:
-- 		- checking the string value for path
-- 		- checking the table supplied for all of it's stuff
-- 		- getting the global userspace variables and copying them
-- 		- creating new tables or arrays for class separation
-- 		- poppping ., .. and any other directory or file names.
-- 		- defining handlers
-- 
-- 
-- @returns
-- --------
-- table
--
-- @end
-- ----
------------------------------------------------------

-- setfenv is the preferred way to do this... _G is going to cause problems.
-- for k,v in pairs(require("core")) do _G[k] = v end

-- setfenv is needed to use global
gloadallinc = 0

-- Clone the global environment once.
local global = {}
for x,y in pairs(_G) do global[x] = _G[x]	end

-- Return the function
return function (dir, ft)
	-- Set up stuff.
	local stat
	local into	= {}
	local lua  	= {lua = dofile}
	local path 	= dir			-- The path to include.
	local rules 	= __rules or {}		-- Rules for file types.
	local exc	= __exclude or {}	-- Files and directories to exclude.
	local inc	= __include or {}	-- Files and directories to include.
	local env	= __env or {}		-- Environment rules (priv, pub, prot)
	local fenv	= __ifenv or {}		-- Directory inclusion rules
	local err  	= __err or {}		-- Handle errors.
	local lvl 	= __levels or 1		-- A depth level.
	local handle 	= __handle or lua	-- A table to attach
	local encdir 	= __dir 		-- How to handle a directory?
	local dirhnd 	= __dirhandler 		-- How to handle a directory?
	
	-- Check that it's a string
	-- tor(dir, "string") -- shut down or error out if no match.
	if type(dir) ~= "string"
	then
		return {
			status = false,
			errv = table.concat{tostring(dir)," is not a parseable string."}
		}
	end

	-- Also check that it's a directory.
	stat = files.stat(dir)
	if stat.status and stat.filetype == "directory"
	then
		-- Get the listing
		t = files.dir(dir)
	
		-- Do stuff.
		if t.status
		then
			-- Check the table for a lot of things.
			ft	= ft or {}
			handle	= ft.rules or { lua = dofile }
			exclude	= ft.exclude	
			environ	= ft.environ

			-- Load each of the classes of functions.
			local fn, bn, en
			for f,ft in ipairs(t.results)
			do
				-- Debugging for t.results
				-- for kk,vv in pairs( ft ) do print(kk, vv) end
				-- render.this(tt)
				stat = files.stat(ft.realpath)
				fp = ft.realpath
				bn = string.gsub(ft.basename, "%." .. ft.extension, "")
				en = ft.extension

				-- Recursively load directories
				if stat.status and stat.filetype == "directory"
				then
					-- Increase count
					gloadallinc = gloadallinc + 1
		
					-- Descend	
					if gloadallinc <= lvl
					then
						-- Success will result in a table of functions.
						-- print(ft.realpath)
						a = loadall(ft.realpath, ft)
						if a.status
						then
							into[bn] = a.results
							gloadallinc = gloadallinc - 1		
						else
							return {
								status = false,
								errv = a.errv
							}
						end
					end
				else
					if handle[en] 
					then
						-- 	x	= dofile(ft.realpath)
						into[bn] = handle[en](fp)
						-- into[bn] = setfenv(handle[en](fp) or 0, global)
					end
				end
			end
	
			return {
				status = true,
				errv = false,
				results = into
			}
		else
			return {
				status = false,
				errv = t.errv
			}
		end
	else
		return {
			status = false,
			errv = table.concat{tostring(dir)," is not a directory."}
		}
	end
end

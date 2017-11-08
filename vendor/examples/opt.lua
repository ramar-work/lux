#!/usr/bin/env lua
------------------------------------------------------
-- @name
-- ------
-- opt - Evaluates options.  
--
-- @synopsis 
-- ----------
-- None so far.
--
-- @description
-- -------------
-- Evaluates options from the command line.
--
-- @options
-- ---------
-- ?
--
-- @usage
-- ----------
-- 
-- Each option looks something like this:
-- long = <long-opt>
-- short = <short-opt>
-- exp = [0-99]
-- valid = <function to validate the value supplied>
-- type = "[nsfd - where n = number, s = string, f = file, d = dir]"
-- 
-- Omitting any fields will skip functionality for that option. 
-- All fields besides 'exp' and 'valid' must be strings.
-- 
-- An option must have at least one field.  'long' is probably a 
-- good choice.
-- 
--
-- @examples
-- ----------
-- 
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
-- @see-also
-- ---------
-- getopt(3)
--
-- @todo
-- -----
-- 	- Indicate whether or not to support dashes
--  	- Provide names of parameters (1 = path, 2 = location)
--	- Support strings as argument maps (vs. just named tables)
--      - Clearer support for values.
-- 	- Use argument flag as table key for value?
--	- 'auto' option to automatically use name of option as long option.
--      - 'autoshort', same for short options.
--      - 'disable_long' / 'disable_short'
--	- Build it into C to keep module maintenance at a minimum
--      - Autonil or autofalse options not specified
-- 
-- @end
-- ----
------------------------------------------------------

-- Old ass requireents...
local function is_value(tval,t,getIndex)
	local status = false
	local index
	if t
	then 
		if t[table.maxn(t)]
		then
			for ind,value in ipairs(t) do
				if tval == value 
				then
					status = true
					index  = ind
				end
			end
		else	
			for ind,value in pairs(t) do
				if tval == value then
					status = true
					index  = ind
				end
			end
		end
	end

	-- ?
	if getIndex then
		return { ["status"] = status, ["index"] = index }
	else
		return status
	end
end

local function is_key(tval,t)
	local status = false
	if not t 
	then
		die.xerror({
			fn = "is.key",
			msg = "Received numerically indexed table at %f. " .. 
			"Expected alphabetically indexed table."
		})
	end

	for key,_ in pairs(t) do
		if tval == key 
		then
			status = true
			return status
		end
	end
end

-- Common codes
local SUCCESS = 0
local INV_FLAG = 1
local INV_TYPE= 2
local NO_ARG = 3
local END_OF_ARG = 5
local INVALID = 4
local PROGRAM
local clargs

-- The option parser
return {

------------------------------------------------------
-- config (t)
--
-- Parse the configuration of a module's options.
------------------------------------------------------
config   = function (t)
	if not t or type(t) ~= "table"
	then
		print("Argument to opts.config() must be a table.")
		os.exit(INV_TYPE)
	end

	clargs = t.clargs
	PROGRAM = (t.program or "<anonymous program>") .. ":"
end,

------------------------------------------------------
-- evaluate (t)
--
-- 
------------------------------------------------------
evaluate = function (t)
	-- Module ought to always be a table.
	if not t or type(t) ~= "table"
	then
		print(PROGRAM, "Argument to opts.evaluate() must be a table.")
		os.exit(INV_TYPE)
	end

	-- An option chain that might not be so hard to work with.
	-- "a:abc:1:path"

	-- Create options table.
	options = { 
		short = {}, 
		long = {}, 
		exp = {}, 
		valid = {},
		type = "-"
	}

	-- Create a map for setting values
	for k,v in pairs(t)
	do
		-- { }.name = k
		t[k] = v 
		v["name"] 			= k
		options.short[v.short or nil] 	= t[k] or false
		options.long[v.long or nil] 	= t[k] or false
		options.exp[v.exp or nil] 	= t[k] or false
	end


	-- Create another table for the status of options
	values = {}

	-- Use opt[name] as the returned processed table.
	c      = 0
	for i=1, table.maxn(clargs)
	do
		-- print("c", c)
		if i > c
		then	
			-- Define differently. 
			n = clargs[i]

			-- Check if it's an option.
			if is_key(n, options.short) 
			then
				-- print(options.short[n].name)			
				current = options.short[n]
			elseif is_key(n, options.long)
			then
				-- print(options.long[n].name)
				current = options.long[n]
			-- If neither of these is set, this option is not real.
			else
				print(PROGRAM, "Option " .. n .. " was not recognized.")	
				os.exit(INV_FLAG)
			end
			

			-- Set it.
			values[current.name] = {}
			(values[current.name]).set = true

			-- Get and validate any arguments.
			if current.exp > 0
			then
				-- Get all the arguments.
				c = i + current.exp
				for x=1, current.exp
				do
					-- Get next argument first.
					next = clargs[i + x] 
					values[current.name]["value"] = {} 
					-- print("ce", current.exp, "arg:", i, "addl (x):", x)

					-- Handle missing arguments. 
					if x == 1 and not next 
					then
						print(PROGRAM, "Option '" .. n .. "' needs an argument.")
						os.exit(END_OF_ARG)
					end

					-- Handle flags when using double dash
					if string.sub(next,0,1) == "-" 
					then
						print(PROGRAM, "Option '" .. n .. "' expects an argument.")
						os.exit(NO_ARG)
					end

					-- Validate all the arguments.
					if current.valid
					then
						-- Status is defined ahead of time.
						values[current.name]["value"][x] = current.valid( next )

						-- If it didn't pass the test, shutdown.
						-- os.exit(INVALID)
					else
						-- ...
						values[current.name]["value"][x] = next 
					end
				end
			end
		end
	end

	-- Return the finished values.
	return values	
end
}


-- Here is the old option module:
------------------------------------------------------
-- unfashionable.lua 
-- 
-- A file for testing and whatnot. 
------------------------------------------------------
-- opt = require("opt")
-- opt.prepare{
-- 	__name = "bla", 	-- Name of thing that's fuckin' up...
-- 	__style = "-", 	-- Use hyphens
-- 	__shift = 1,		-- Autoshift up by one?
-- 	__errv  = function ()
-- 		-- The more I see this, I see an event-based
-- 		-- device.
-- 	end,
-- 	a = { "a", "do-this", 1, function () return a end, }
-- 	-- long-a = t.a,
-- 	b = { "a", "do-this", 1, function () return a end, }
-- 	-- long-b = t.b,
-- 	b = function () return b end,
-- 	b = function () return b end,
-- }
-- 
-- -- Stop
-- while {...} > 1
-- do
-- 	index 	= index or 1
-- 	call	= opt.get({...}[index]) 
-- 
-- 	-- Handles bad options
-- 	if not call.found 
-- 	then
-- 		print("Unknown option: ", call.arg)
-- 		os.exit(1)
-- 
-- 	-- Handles wrong number of arguments
-- 	elseif call.found ~= call.expects
-- 	then
-- 		print(call.arg, " expects ", call.expects, " options.")
-- 		print("Received ", call.found, ".")
-- 		os.exit(1)
-- 
-- 	-- Whatever validation is in place failed.
-- 	elseif not call.valid
-- 	then
-- 		print(call.arg " expects " ... )
-- 		os.exit(1)	
-- 
-- 	-- This block can be extended heavily.
-- 	-- Suppose the validation failed and I want a different message.
-- 	else
-- 		-- opt.advance
-- 	end
-- 
-- 	-- Advance pointer within (or reduce total array of) arguments
-- 	table.remove({...}, index)	-- or
-- end

#!/usr/bin/env lua
------------------------------------------------------
-- @name
-- ------
-- opttest - Here's a quick test of the Lua option
-- module.
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
-- @examples
-- ----------
-- None so far.
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
-- - Should be able to work with stdin
-- - Indicate whether or not to support dashes
--
-- @end
-- ----
------------------------------------------------------
opt = require("opt")

opt.config{
	double_dash = true,
	validator   = function ()
		-- This validator function makes sure
		-- that all command-line arguments are
		-- strings.
	end,
	clargs 	    = {...},
	name	    = "opttest"
}

opts = opt.evaluate{
	crime = { 			-- Always name the option argument
		short = "-c", 		-- Short option if there is one...		
		long  = "--illegal",  	-- Long option
		exp   = 0,		-- Number of arguments expected
	},
	title = { 			-- Always name the option argument
		short = "-a", 		-- Short option if there is one...		
		long  = "--name",  	-- Long option
		exp   = 1,		-- Number of arguments expected
	},

	path = { 			-- Always name the option argument
		short = "-p", 		-- Short option if there is one...		
		long  = "--path", 	-- Long option
		exp   = 1,		-- Number of arguments expected
	   	valid = function (x)	-- A validator.
			-- This checks that the path is a path. 
			return x
		end
	}
}

-- Somewhat close functionality...
-- if path.set then print(path.value) end

for k,v in pairs(values) do 
print(k, "set:", v.set)  
if v.value then
print(v.value[1])
end
end

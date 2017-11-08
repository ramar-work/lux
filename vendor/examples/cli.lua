#!/usr/bin/env lua

--[[
--CLI Interface (v.00000000.1)
--
--Created: 	2011-10-11 8:05 AM
--Author: 	Antonio Ramar Collins II (ramar.collins@gmail.com)
--Company:	Vokay Ent....
--
--Usage:		Define tables a, val & opts.
--				a 		= command line arguments
--				val	= final values (to be passed into script for execution)
--				opts	= possible options
--
--				Tables 'val' and 'opts' are referenced by the element within the script
--				or module that they control.
--				
--				E.g. "opts.deploy" refers to the possible flags, expected args and default value.
--				opts.deploy == { "flag1", "flag2", "args_expected", "default" }
--				A good convention is to put short flags in flag1 and long in flag2.
--
--				val.deploy holds the value or values of a command line arg if one exists.
--				When there are two or more command line args, lua makes a table to hold all of them.
--
--Synopsis:	Allows lua to emulate a getopts-like shell interface that supports long
--				options.  
--
--More:		(Step-by-Step)
--				1) Lua first defines an argument table (a) to store user values.
--				2) Then we define what options (opts) are available for our program/module.
--				3) For (alleged) speed, we take our command-line flags from the options and 
--				put them in a table called (flags). (The assumption is that there is less to 
--				compare and therefore compute.)
--				4) Then we set our values based on what we find in opts. 
--				For example: 
--					If c[2] 	= 0, the flag accepts no arguments -- set value at the end of c 
--						table.
--  				If c[2]   >= 1, the flag accepts c[2] arguments)
--  			5) Finally, we have some testing logic that should change depending on what
--				we use as options.
--
--ToDo:		Autohelp (or at least ability to link in a custom function to help)
--				Failures (custom messages most likely are best)
--]]

--Come up with something scientific to say here :)
a 		= { ... }	--User Arguments table.
val	= {}			--Table to hold values for our module.

--Module Options table. (Can be included at runtime.)
opts 	= { 
	deploy 	= { "-d" , "--deploy", 	0,	true}, 			-- Deals with something
	dryrun 	= { "-r" , "--dry-run", 0,	true}, 			-- Says something is dry-run. 
	config  	= { "-g" , "--config" , 1,	'file.cfg'}, 	-- Expects 1 argument
	network	= { "-n" , "--stuff" , 	3,	''}, 				-- Expects 3 arguments.
	locale	= { "-l" , "--locale",  1, 'english'}  	-- ???
}

---[[
-- function setuv(opt, optindex, table)
--
-- var 	opt
-- int 	optindex
-- table table
--
--	Takes a switch (opt) for user arguments table, that switch's index (optindex), 
--	and a table of values to grab (table) to set options for a script.
--
function set_user_values(opt, optindex, table) 
	--If opt is in c (c[1] or c[2]), take c[3] and set val.c = User Arg
	for v, t in pairs(table) do
		if opt == t[1] or opt == t[2] then
			-- If n args = 0, simply set the flag.
			if t[3] == 0 then
				val.v = t[4] 
				print(val.v)
			-- If n args = 1, set whatever the arg is.
			elseif 	t[3] == 1 then
				uv	= tonumber(optindex + t[3])
				val.v = a[uv] 
				print(val.v)
			-- If n args > 1, hold the args in a table.
			elseif 	t[3] > 1  then
				val.v = {} 
				function f()
					c, uv = 1, optindex + 1 	
					while uv <= (optindex + t[3]) do
						val.v[c] 	= a[uv]
						c, uv = c + 1, uv + 1
					end
					--[[
					for _,v in ipairs(val.v) do
						print('Our new table '..v)
					end
					--]]
				end
				f()
			else --Do something for negative indices...	
			end
		else
		end	
		--]]
	end
end
--]]

---[[
--Option Processing.
for i,v in ipairs(a) do
	set_user_values(v,i,opts)
end
--]]
--
--[[
--Fill in the blanks.
--
--if no user value is set, 
--set the defaults (4th value).
--

--]]


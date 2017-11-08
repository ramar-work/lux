#!/usr/bin/lua
--
-- setopt.lua
--
-- Created: 	2011-10-11 8:05 AM
-- Author: 		Antonio Ramar Collins II (ramar.collins@gmail.com)
-- Company:		Vokay Ent. (vokayent.com)
-- Revision: 	0.1
-- Usage:		Easy :)
--
--					Seriously,
-- 				o = require("setopts")
-- 				o.setopt(uargs,mod)
--
-- 				uargs = {...}, the customary Lua declaration for args[1-x]
-- 				file	= x.lua, where x contains two tables, (val) and (opts).
--
-- Usage(Long):(val) denotes the actual values that will be used from your script.
-- 				(opts) denotes possible options for your script written in the form:
-- 				index = { 'short opt' (string), 'long opt' (string), args expected (integer), 'default value' (string or boolean if args expected = 0) }
--
-- 				With only one file acting as the script, uargs will equal {...} and file will equal a table holding (val) and (opts)
-- 				Something like:
--					a 	 = { ... }
-- 				mod = { 
-- 					opts = { n1, n2 ... nx }
-- 					val  = { }
-- 				}
--					setopt(a,mod)-
-- 				would probably do the trick. 

--
--setopt(uargs,sopts)
--
--Compares user arguments (uargs) against possible options and values in a script (mod) and sets accordingly.
--
local setopts = {}

local function setopt(uargs,mod) 
	-- Get each element in user args.
	for optindex, optvalue in ipairs(uargs) do
		opt = {}
		--print('Type '..type(opt)..', Index: '..optindex..', Value:'..optvalue)
		for v, t in pairs(mod.opts) do
			opt = {
				["short"] 	= t[1],
				["long"] 	= t[2],
				["ae"] 		= t[3], --Arguments expected.
				["default"] = t[4]
			}
		--for i,n in pairs(opt) do print(i, n) end
			--if optvalue == t[1] or optvalue == t[2] then
			if optvalue == opt.short or optvalue == opt.long then
				-- If n args = 0, simply set the flag.
				print(v)
				print(type(mod.val))
				if opt.ae  == 0 then
					mod.val.v = opt.default -- val.v is s 
					print('No Arg: '..'"'..v..'"'..' flag is set.')
				-- If n args = 1, set whatever the arg is.
				elseif 	opt.ae == 1 then
					uv	= tonumber(optindex + opt.ae)
					mod.val.v = uargs[uv] 
					print('1 Arg '..mod.val.v)
				-- If n args > 1, hold the args in a table.
				elseif 	opt.ae > 1  then
					mod.val.v = {} 
					c, uv = 1, optindex + 1 	
					while uv <= (optindex + opt.ae) do
						mod.val.v[c] = uargs[uv]
						c, uv 		 = c + 1, uv + 1
					end
					print(#mod.val.v..' Args:')
					for _,mvv in ipairs(mod.val.v) do
						print(mvv)
					end
		--[[
				else --Do something for negative indices...	
		--]]
				end --endif
			else
			end --endif
		end --endfor
	end --endfor
end

setopts = {
	['setopt'] = function(x,y) setopt(x,y) end
}

return setopts

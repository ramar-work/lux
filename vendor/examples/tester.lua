#!/usr/bin/env lua
------------------------------------------------------
-- tester.lua 
-- 
-- I already wrote this in another module.  It's ok
-- to keep this here.
--
-- It can't do C testing.
------------------------------------------------------
require("core")
local fn
-- uc.errtest()  -- Error test

---[[
uc.gett3{
	name = "gett2",   -- Tests a key that should be found.
	linux_v = "Mint 2.4", -- Tests a key that should not exist.
	amun = 1248, -- Tests a key that should not exist.
	network = "192.168.1.23", -- Tests a key that is required.
	nlayers = { "abc", 45, "tennyson"  }, -- Tests nested tables
	klayers = { abc = "def", ghi = 234, fun = function (x) return x end },
}
--]]

os.exit(0)

-- Key testing.
uc.gett{
	name = "Tammybro",
	hdd = 324,
	network = "129.233.09.12",
}


-- Hash testing
-- if table.maxn({...}) > 0 then tests.kt(({...})[1], ({...})[2]) end
-- os.exit(0)


-- Run some tests with the filetests directory
inc = 1
for x, y in pairs(files.dir("filetests").results)
do
	-- Move through each file.
	print(inc)

	-- Stats
	fn = y.basename
	print("Running test with " .. fn)
	print("-----------------------")
	print("Size of file:   ", y.size)
	print("Links to file:  ", y.links)
	print("Owner:          ", y.uid)

	-- Load it up
	r_in = files.fread(y.realpath, 1, 1)
	if r_in.status
	then
		-- Status
		print("'" .. fn .. "' successfully opened.")

		-- Details
		print("File payload: ")
		print("-----------------------")
		print("Type:   ", type(r_in.body))
		print("Size:   ", table.maxn(r_in.body))
		
		-- Byte by byte write to file (this is silly...)
		--[[
		new_f = io.open("repro/copy_" .. fn, "w+")
		for k,v in ipairs(r_in.body)
		do
			new_f:write(v)
		end
		new_f:close()
		--]]
		
		-- Full at once write.
		-- io.output("repro/" .. fn):write(table.concat(r_in.body))

		-- Try a copy
		files.fcopy(y.realpath, "repro/now_" .. y.basename)
	else
		-- Handle error cases
		print("Error loading '" .. y.realpath .. "'.")
		print(r_in.errv)
	end	

	-- Formatting
	print("\n")
inc = inc + 1
end


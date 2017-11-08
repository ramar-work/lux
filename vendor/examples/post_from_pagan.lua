------------------------------------------------------
-- post.lua
--
-- Process POST variables and stuff. 
------------------------------------------------------
local PST
return function(CGI,CLI)
	------------------------------------------------------
	-- Process "application/x-www-url-form" encoded forms. 
	------------------------------------------------------
	if CGI.CONTENT_TYPE == "application/x-www-form-urlencoded"
	then
		-- Grab the raw stream.
		PST = io.stdin:read(tonumber(CGI.CONTENT_LENGTH))
		local post_table = {}

		-- Do multiple values exist?
		if string.find(PST,'&')
		then

			-- Break the stream up into manageable portions.
			for _,val in ipairs(table.from(PST,'&'))
			do
				local pp = string.chop(val,'=')
				post_table[pp.key] = string.decode(pp.value)
			end	

			return post_table

		-- If not, Create and return a single key table.
		else

			local pp = string.chop(PST,'=')
			return {[pp.key] = string.decode(pp.value)}

		end	

	------------------------------------------------------
	-- Process "multipart/form-data" encoded forms. 
	------------------------------------------------------
	elseif string.find(CGI.CONTENT_TYPE,"multipart/form-data",1,true)
	then
		PST = io.stdin:read(tonumber(CGI.CONTENT_LENGTH - 1)) 
	
		-- Debugging
		-- F.asset("private")
		-- F.write(PST,"post.log")

		-- Extract the boundary.
		-- Only one single boundary is chopping right now.
		local START,END,c = {},{},1
		local BOUNDARY = string.sub(CGI.CONTENT_TYPE,({ 
			string.find(CGI.CONTENT_TYPE,
				"multipart/form-data; boundary=",1,true)})[2] + 1)


		-- You can easily have two or more boundaries 
		-- depending on what's in the page.
		for W in string.gmatch(PST,BOUNDARY)
		do
			START[c],END[c] = string.find(PST, BOUNDARY, END[c-1] or 1, true)
			c = c + 1
		end

		-- Define some placeholders.
		local s,ct = {},{}
		local BLOCK

		-- Get the content disposition markers. 
		for x=1,table.maxn(START),1 
		do
			-- Define content and boundaries for this iteration. 
			local BLOCK = {
				["CONTENT"] = string.sub(PST,
					(END[x] + 1), ((START[x + 1] or -1) - 4)),
				["START"] = (END[x] + 1),
				["END"] = (START[x + 1] or -1),
			}
			
			-- Find the Content-Disposition tag.
			local FDSTRING = ({ 
				string.find(BLOCK.CONTENT,
					"Content-Disposition: form-data;",1,true) 
			})[2]

			-- If we've got form-data, process it.
			if FDSTRING
			then
				local cxc = string.sub(BLOCK.CONTENT, 
					FDSTRING + 1,(BLOCK.END - 4))
				
				if string.match(cxc,"filename=",1,true)
				then
					ctstart = ({ 
						string.find(cxc,
							"Content%-Type: ([%w%p%d]+/[%w%p%d]+)\r")
					})[2]

					-- Catch name field.
					ct[string.match(cxc,'name="([%w%p%d]+)"')] = {
						["filename"] = string.match(cxc,
							'filename="([%w%p%d ]+)"\r'),

						["contenttype"] = string.match(cxc,
							"Content%-Type: ([%w%p%d]+/[%w%p%d]+)"),

						["content"] = string.sub(cxc,
							ctstart + 4,(BLOCK.END - 6))
					}
				else
					-- According to spec, get 
					-- spaces
					-- ., _, -,:,digits

					ct[string.match(cxc, 'name="([%w%d,_,%-,:,%.]+)"')] = string.sub(
						string.gsub(
							string.gsub(
								string.gsub(cxc,'name="([%w%d,_,%-,:,%.]+)"',""),
									"\r\n",""),
								"\r",""),2)
				end
			elseif { 
				string.find(BLOCK.CONTENT,"Content-Disposition: file;",1,true) 
			} then
				local ff = { string.find(BLOCK.CONTENT,
					"Content-Disposition: file;",1,true) }
			end
		end
		return ct
	end
end

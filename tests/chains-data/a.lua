-- a router would look somewhat like this
--[[
This router module checks a lot of things...

/spaghetti:
	should return only when 'id' and 'talks' are there,
		can only be in GET or POST using 'on'
	var may also be there, but its default is 322
	'you' should not be allowed, and the initial check should fail
		(throw a 500 obviously) because its a function

	/spaghetti/sauce:
		should say 'sauce is too sweet!'

	/spaghetti/oregano:
		should say "oregano makes me happy"

	/spaghetti/sausage:
		should say "Shimmy shimmy shimmy, you need meat in your pasta for protein",

	/spaghetti/pie:
		should return the result of abc.lua and def.lua and interpreted via 'potato'

/hotdogs:
	should return "I'll do your bacon and cook your grits!"	

/pizza:
	should return "Order in with Pizza Hut"

/sushi:
	should return "<h2>abc</h2>..."

/falafel:
	should return results of {greek,2,gyro}.lua, and interpreted with 'malicious'

/padthai
	should return results of spicy.lua and an anonymous function, and interpreted with {aaron,rodriguez}.lua

*
	this should match EVERYTHING else...

[/.../.../]
	this should match every matching regex...

--]]

function abc ( )
	return "<h2>abc</h2><p>Now Hiring</p>"
end

-- Must be an explicit return
return {
	spaghetti = {
		sauce = "sauce is too sweet!",

		oregano = function () 
			return "oregano makes me happy"
		end,

		sausage = "Shimmy shimmy shimmy, you need meat in your pasta for protein",

		pie = {
			model = { "abc", "def" },
			view  = "potato"
		},
		
		expects =  { 
			on    = { "POST" , "GET" },
			id    = "number", 
			talks = "string",	
			var   = 322,
			you   = function ( a )
				-- Check the parameter somehow...
				return a
			end	
		}
	},

	hotdogs = function () 
		return "I'll do your bacon and cook your grits!"
	end,
	
	pizza = "Order in with Pizza Hut",

	sushi = abc,

	falafel = {
		model = { "greek", "2", "gyro" },
		view  = "malicious"
	},

	padthai = {
		model = { "spicy", function() return 'abcdefg' end  },
		view  = { "aaron", "rodriguez" }
	},

	["*"] = "wildcard",

	["/asdfsadf/asdfjsadf"] = "regex"
}


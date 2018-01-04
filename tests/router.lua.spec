#!/usr/bin/env lua
[[---------------------------------------
router.lua
==========

# Issues with the C router 

- Hash tables suck:
	Getting false positives (or maybe trying to query the wrong data)
	for example:	
		falafel.cd will sometimes return true
		so will falafel.<word>.<word>.model and this isn't			
			true in Lua...


- I remember seeing somewhere that Lua doesn't
	necessarily enforce any kind of order on tables.
	I don't know if this is a serious issue, because
	if the Table implementation worked correctly, it
	wouldn't matter how the key order fell
		
	Either way, it does cause a problem when getting
	things to work correctly...
		

- So narrowing that down, I can make a little issue
	list and start marking off the things that may or 
	may not be an issue

	x table order
			(not an issue b/c it does work most of the time)
		table value retrieval
			does not always work, why?	
		notice that userdata (nor threads) are acounted for
			this COULD relate to the above, very thin chance though, no obvious c&e



# A Spec


----------------------------------------]]





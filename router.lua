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

return {
	routeName = [ <function>, <string>, <table> ],

	if routeName points to a <table> 
	then
		any of the following are keys that have meaning:
			*          = wildcard that accepts anything
			/.../      = a regular expression that accepts anything
			model      = <string>, <function> or <table> that defines a model
			view       = <string>, <function> or <table> that defines a view 
			expects    = <table> that defines what is allowed should the url STOP here
								   (notice this means that any level deeper with a match will throw
								    these rules out)
			*file      = <string> that points to a binary file that should be served
			[*301/302] = <string> that directs where a user should be redirected
			*query     = <string> that points to a query
			*queryfile = <string> that points to a file containing a query
			
			**NOTE: query and query file could use arguments and checking

		other rules:
			any name not matching a regex (like 'sally' for instance) will pull up 
			the associated value if the URL contains a match.  So if:
			<value>   = <string> 
				The engine will return a string (or binary content, since the
				engine deals with this particular string field as uint8_t)
			<value>   = <function> 
				The engine will execute the function and return the payload 
			<value>   = <table>
				The engine will search for other matches according to the above rules
				at the next part of the URL.
	fi			
}

----------------------------------------]]





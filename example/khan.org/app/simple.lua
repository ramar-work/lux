-- simple.lua
--
-- This exists just to check that things work.
--

print( env.headers.Accept );
return {
	frosty = "Santa"
 ,accept = env.Accept
 ,host   = env.Host
}

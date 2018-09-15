# Usage

As I come to a close on the development of this project, I'm going to place some notes throughout that may help actually get this thing running successfully.


## Table of Contents

...


## Sections

### data.lua

- When returning routes, always use a key titled 'routes' (for now).  Without it, hypno will, at best, not serve the requested page.  And at worst, crash.
<pre>
	Example:
		return {
>>>>	routes = {
				...
			}
		}	
</pre>




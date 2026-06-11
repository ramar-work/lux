# ----------------------------------------------------------
# docs.mk
# =======
# Recipes to handle creating Lux's documentation.  
#
#
# Usage
# -----
# `docs` - Create documetation
# `docs-check` - Check for tools we'll absolutely need to generate documentation
# `docs-config` - Generate static & configuration files that can improve site performance.
# `docs-dirs` - Create a temporary directory and a directory for local documentation
# `docs-local` - Generate local HTML documentation only 
# `docs-pages` - Generate complete pages for each Lua module
# `docs-index` - Generate a home page for the documentation
# `docs-nav` - Create the general navigation
# `docs-lua` - Creates HTML pages for lux's Lua extensions 
# `docs-lua-hash` - Having issues with just this one file...
# `docs-clean` - Remove the temporary directory for templates 
# `docs-complete` - Let the user know that generation was successful
# 
#
# Details 
# --------
# All of this is about as old school and low-tech as we can get.
# The idea is to keep it simple and allow users to do local 
# searches without things breaking on both the internet and a 
# regular filesystem.
#
# These are simple scripts that work by extracting comments from 
# source files, and repackaging them into HTML files.  
#
# Users of MacOS and Windows may have some trouble due to 
# differences in the text processing software running behind 
# the scenes.
#
#
# Caveats
# --------
# The `docs-lua` target should be much less complicated than
# what it is.  The `docs-lua-hash` tagrget is a workaround for 
# a strange bug I'm having with the file called 'hash.c'.  
# This is the only file that doesn't seem make it through the 
# markdown filter, and I'm wondering if it has to do with the 
# `hash` built-in of bash shell...
#
# ----------------------------------------------------------
TMPDIR=tmp
DOCDIR=www/docs
DOCDOMAIN=luxserver.org
DATE=$(shell date +%F)


# docs - Create documetation
docs: docs-check docs-dirs docs-local docs-config docs-complete


# docs-check - Check for tools we'll absolutely need to generate documentation
docs-check:
	@echo "Checking for existence of markdown cli tool:"
	markdown -version


# docs-dirs- Create a temporary directory and a directory for local documentation
docs-dirs:
	-test -d $(TMPDIR)/ && rm -rf $(TMPDIR)
	mkdir $(TMPDIR)/
	test ! -d $(DOCDIR)/ && mkdir $(DOCDIR) || echo "$(DOCDIR)/ exists... skipping."


# docs-local - Generate local HTML documentation only 
docs-local: docs-pages


# docs-pages - Generate complete pages for each Lua module
docs-pages: docs-nav docs-lua docs-index
	@test -d $(DOCDIR)/
	@find src/lua/ -type f \
		-name "*.c" ! -name "lib.c" ! -name "lua.c" ! -name "session.c" | \
	xargs -IFILE sh -c ' \
		{ \
		FF=`basename FILE | sed "s/\.c/.html/"`; \
		printf "<html>\n"; \
		printf "<head>\n"; \
		printf "<link rel=\"stylesheet\" href=\"./index.css\">\n"; \
		printf "<script src=\"./index.js\"></script>\n"; \
		printf "</head>\n"; \
		printf "<body>\n"; \
		printf "<header>\n"; \
		printf "</header>\n"; \
		printf "<div id=top>\n"; \
		printf "	<div>\n"; \
		printf "		lux v$(VERSION)\n"; \
		printf "	</div>\n"; \
		printf "	<div>\n"; \
		printf "		<input name=\"search\">\n"; \
		printf "		<input type=\"submit\" value=\"Search\">\n"; \
		printf "	</div>\n"; \
		printf "</div>\n"; \
		printf "<div id=nav>\n"; \
		printf "<ul>\n"; \
		cat $(TMPDIR)/sidenav.html; \
		printf "</ul>\n"; \
		printf "</div>\n"; \
		printf "<div id=description>\n"; \
		cat $(TMPDIR)/$${FF}; \
		printf "</div>\n"; \
		printf "<footer>\n"; \
		printf "	<div>Copyright `date +%Y`, All Rights Reserved</div>\n"; \
		printf "	<div>Site Design by <a href=\"https://ramar.work\">rwk</a></div>\n"; \
		printf "	<div>You are viewing documentation for the <a href=\"https://luxserver.org\">lux</a> web server</div>\n"; \
		printf "	<div><a href=\"./sitemap.xml\">Sitemap</a></div>\n"; \
		printf "</footer>\n"; \
		printf "</body>\n"; \
		printf "</html>\n"; \
		} > $(DOCDIR)/`basename FILE | sed "s/\.c//"`.html; \
	'


# docs-index - Generate a home page for the documentation
docs-index:
	@test -d $(DOCDIR)/;
	@for n in index.html; do \
		{ \
		printf "<html>\n"; \
		printf "<head>\n"; \
		printf "<link rel=\"stylesheet\" href=\"./index.css\">\n"; \
		printf "<script src=\"./index.js\"></script>\n"; \
		printf "</head>\n"; \
		printf "<body>\n"; \
		printf "<header>\n"; \
		printf "</header>\n"; \
		printf "<div id=top>\n"; \
		printf "	<div>\n"; \
		printf "		lux v$(VERSION)\n"; \
		printf "	</div>\n"; \
		printf "	<div>\n"; \
		printf "		<input name=\"search\">\n"; \
		printf "		<input type=\"submit\" value=\"Search\">\n"; \
		printf "	</div>\n"; \
		printf "</div>\n"; \
		printf "<div id=nav>\n"; \
		printf "<ul>\n"; \
		cat $(TMPDIR)/sidenav.html; \
		printf "</ul>\n"; \
		printf "</div>\n"; \
		printf "<div id=description>\n"; \
		printf "<h2>documentation</h2>\n"; \
		printf "<p>lux docs homepage</p>\n"; \
		printf "<p>Feel free to explore, search for terms and find ways to build what you want with lux.</p>\n"; \
		printf "</div>\n"; \
		printf "<footer>\n"; \
		printf "	<div>Copyright `date +%Y`, All Rights Reserved</div>\n"; \
		printf "	<div>Site Design by <a href=\"https://ramar.work\">rwk</a></div>\n"; \
		printf "	<div>You are viewing documentation for the <a href=\"https://luxserver.org\">lux</a> web server</div>\n"; \
		printf "	<div><a href=\"./sitemap.xml\">Sitemap</a></div>\n"; \
		printf "</footer>\n"; \
		printf "</body>\n"; \
		printf "</html>\n"; \
		} > $(DOCDIR)/index.html; \
	done	


# docs-nav - Create the general navigation
docs-nav:
	@test -d $(TMPDIR)/;
	@printf "\n" > $(TMPDIR)/sidenav.html;
	@find src/lua/ -type f -name "*.c" ! -name "lib.c" ! -name "lua.c" ! -name "session.c" | \
		sort | \
		xargs -IFF sh -c ' \
			grep -A 1 "/\*\*" FF | \
			sed "{ \
				s/^ \* //; \
 				2s|\(.*\)\.c|<li>\n\t<a href=\"\1.html\">\1</a>\n\t<ul>|; \
				s|^\([a-z]*\).\([a-z0-9]*\) (.*)|\t\t<li><a href=\"\1.html#\1.\2\">\1.\2</a></li>|; \
 				/\/\*\*/d; \
 				/--/d; \
			}"; \
			printf "\t</ul>\n</li>\n" \
		' >> $(TMPDIR)/sidenav.html


# docslua - Creates HTML pages for lux's Lua extensions 
docs-lua: docs-lua-hash
	@test -d $(TMPDIR)/;
	@find src/lua/ -type f -name "*.c" ! -name "hash.c" ! -name "lib.c" ! -name "lua.c" ! -name "session.c" | \
		xargs -IFF sh -c ' \
			PATH="$$PATH"; \
			FILE=`basename FF` && \
			HTML="$(TMPDIR)/$${FILE%.c}.html" && \
			sed -n "{ /^\/\*\*/p; /^ \*/p; }" FF | \
			sed "{ s|^/\*\*|<p class=\"top\"></p>|; }" | \
			sed "{ s|^ \*/|<p class=\"bot\"></p>|; }" | \
			sed "{ s|^ \* ||; s|^ \*||; }" | \
			sed "2s/\.c//" | \
			markdown | \
			sed "{ s|<p class=\"top\"></p>|<div class=\"section\">| }" | \
			sed "{ s|<p class=\"bot\"></p>|</div>| }" | \
			sed "{ s|<h2>\([A-Za-z0-9.]*\)|<h2 id=\"\1\">\1| }" | \
			sed "{ s|<h2 id=\"Usage\">|<h2>| }" | \
			sed "{ s|<h2 id=\"Examples\">|<h2>| }" | \
			sed "{ s|<h2 id=\"Caveats\">|<h2>| }" > $${HTML} \
		'

# docs-lua-hash - Silly workaround for hash.c
docs-lua-hash:
	@test -d $(TMPDIR)/
	@sed -n "{ /^\/\*\*/p; /^ \*/p; }" src/lua/hash.c | \
		sed "{ s|^/\*\*|<p class=\"top\"></p>|; }" | \
		sed "{ s|^ \*/|<p class=\"bot\"></p>|; }" | \
		sed "{ s|^ \* ||; s|^ \*||; }" | \
		sed "2s/\.c//" | \
		markdown | \
		sed "{ s|<p class=\"top\"></p>|<div class=\"section\">| }" | \
		sed "{ s|<p class=\"bot\"></p>|</div>| }" > $(TMPDIR)/hash.html


# docs-clean - Remove the temporary directory for templates 
docs-clean:
	-@rm -rf $(TMPDIR)/


# docs-config - Generate files improving site performance
docs-config:
	printf 'return { cache = { ["/assets"] = "max-age=86400, immutable" } }' > $(DOCDIR)/config.lua
	printf 'Disallow: *' > $(DOCDIR)/robots.txt
	printf 'Disallow: *' > $(DOCDIR)/llms.txt
	{ \
	printf '<?xml version="1.0" encoding="UTF-8"?>\n'; \
	printf '<urlset xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n'; \
	printf '\txsi:schemaLocation="http://www.sitemaps.org/schemas/sitemap/0.9 http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd"\n'; \
	printf '\txmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'; \
	printf '\t<url>\n'; \
	printf '\t\t<loc>https://$(DOCDOMAIN)/</loc>\n'; \
	printf '\t\t<lastmod>$(DATE)</lastmod>\n'; \
	printf '\t\t<changefreq>weekly</changefreq>\n'; \
	printf '\t\t<priority>0.8</priority>\n'; \
	printf '\t</url>\n'; \
	printf '</urlset>\n'; \
	} > $(DOCDIR)/sitemap.xml


# docs-complete - Let the user know that generation was successful
docs-complete: docs-clean
	@printf "Documentation generation was successful.\n" 
	@printf "You can see the index page at file://`pwd`/$(DOCDIR)/index.html\n"

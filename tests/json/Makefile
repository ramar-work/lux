# Lua and zjson test suite?
# NOTE: The best way to test these is with by simulating an environment from C.
# This is the way that hypno runs, and is the best way to test for leaks.
CC=gcc
SRCPATH=../../vendor
LIBDIR=.libs
IFLAGS=-I$(SRCPATH) -I$(SRCPATH)/lua-5.4.3/src
SHOULD_SUCCEED= \
	array-with-empty-object.json \
	array-with-empty-objects.json \
	array-with-two-populated-objects.json \
	empty-array-with-whitespace.json \
	empty-array.json \
	empty-object-with-whitespace.json \
	empty-object.json \
	escaped-string.json \
	object-with-array-of-empty-objects.json \
	object-with-empty-array.json \
	object-with-empty-object.json \
	simple-object-1.json \
	simple-object-2.json \
	clothing-large.json \
	clothing-small.json \
	embedded-single-quotes.json

# test - Run each of the success tests against Lua
test: lib
	for n in $(SHOULD_SUCCEED); do \
		export PATHVAR="../$${n}"; \
		lua -l json < ./json.lua; \
		read; \
	done

# lib - Compile and test the JSON library 
lib:
	-@rm -rf $(LIBDIR)
	-@mkdir -p $(LIBDIR)
	$(CC) -fPIC -o $(LIBDIR)/ztable.o -c $(SRCPATH)/ztable.c $(IFLAGS)
	$(CC) -fPIC -o $(LIBDIR)/zwalker.o -c $(SRCPATH)/zwalker.c $(IFLAGS)
	$(CC) -fPIC -o $(LIBDIR)/zjson.o -c $(SRCPATH)/zjson.c $(IFLAGS)
	$(CC) -fPIC -o $(LIBDIR)/lua.o -c ../../src/lua.c $(IFLAGS)
	$(CC) -DLUA_LOPEN -fPIC -o $(LIBDIR)/json.o -c ../../src/lua/json.c $(IFLAGS)
	$(CC) -shared -o $(LIBDIR)/json.so $(LIBDIR)/ztable.o $(LIBDIR)/zjson.o $(LIBDIR)/lua.o $(LIBDIR)/json.o $(LIBDIR)/zwalker.o

# ...
y: lib
	export PATHVAR="../clothing-small.json"; \
	cd .libs && lua -l json < ../json.lua

# 
clean:
	rm -rf $(LIBDIR)

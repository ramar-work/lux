# This project...
NAME = hypno
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c main.c
OBJ = ${SRC:.c=.o}
LLVMS=/usr/local/Cellar/llvm/5.0.0/bin/llvm-symbolizer

# Debian seems to need this
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)

GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O0 -DDEBUG_H #-ansi
CC = gcc
CFLAGS = $(GCCFLAGS)

# (just get rid of this target when the tests are done)
first: main 
	printf '' >/dev/null
	
# A main target, that will most likely result in a binary
main: $(OBJ)
	@echo $(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua

# Router test program
router: RICKROSS=testparser
router: test-build
router: 	
	printf ''>/dev/null

# Chain test program
chains: RICKROSS=testchains
chains: test-build
chains: 
	printf ''>/dev/null

# All test build programs use this recipe
test-build:
	@echo $(CC) -c $(CFLAGS) vendor/single.c vendor/nw.c vendor/http.c bridge.c $(RICKROSS).c vendor/sqlite3.c
	@$(CC) -c -DSQROOGE_H $(CFLAGS) vendor/single.c vendor/nw.c vendor/http.c bridge.c $(RICKROSS).c vendor/sqlite3.c
	@echo $(CC) $(CFLAGS) single.o nw.o http.o bridge.o $(RICKROSS).o sqlite3.o -o $(RICKROSS) -llua
	@$(CC) $(CFLAGS) single.o nw.o http.o bridge.o $(RICKROSS).o sqlite3.o -o $(RICKROSS) -llua


osx: 
	@echo $(CC) -c -DSQROOGE_H $(CFLAGS) vendor/single.c vendor/nw.c vendor/http.c bridge.c tests.c 
	@$(CC) -c -DSQROOGE_H $(CFLAGS) vendor/single.c vendor/nw.c vendor/http.c bridge.c tests.c 
	@echo $(CC) $(CFLAGS) single.o nw.o http.o bridge.o tests.o -o ho -llua
	@$(CC) $(CFLAGS) single.o nw.o http.o bridge.o tests.o -o ho -llua

linux: 
	@echo $(CC) -c -DSQROOGE_H $(CFLAGS) vendor/single.c vendor/nw.c vendor/http.c bridge.c tests.c 
	@$(CC) -c -DSQROOGE_H $(CFLAGS) $(shell pkg-config --cflags lua5.3) vendor/single.c vendor/nw.c vendor/http.c bridge.c tests.c 
	@echo $(CC) $(CFLAGS) single.o nw.o http.o bridge.o tests.o -o ho $(shell pkg-config --libs lua5.3)
	@$(CC) $(CFLAGS) single.o nw.o http.o bridge.o tests.o -o ho $(shell pkg-config --libs lua5.3)

	
# A not-so-main target, that will probably result in a few object files...
objs: $(OBJ)
	@printf "Finished building objects\n"


# A packaging target
pkg:


# An update target?  (checks for new versions of all your vendor dependencies...)	


# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) 
	-@find . | egrep '\.o$$' | xargs rm

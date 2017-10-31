# This project...
NAME = hypno
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c main.c
OBJ = ${SRC:.c=.o}
LLVMS=/usr/local/Cellar/llvm/5.0.0/bin/llvm-symbolizer

# Debian seems to need this
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O0 -DDEBUG_H
CC = gcc
CFLAGS = $(GCCFLAGS)

CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)

# (just get rid of this target when the tests are done)
first: osx
	printf '' >/dev/null
	
# A main target, that will most likely result in a binary
main: $(OBJ)
	@echo $(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua

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

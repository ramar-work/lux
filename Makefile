# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O0 #-DDEBUG_H #-ansi
CC = gcc
CFLAGS = $(GCCFLAGS)

# Some Linux systems need these, but pkg-config should handle it
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c
OBJ = ${SRC:.c=.o}


# A main target, that will most likely result in a binary
main: RICKROSS=main
main: test-build-$(OS)
main: 
	mv $(RICKROSS) hypno


# Router test program
router: RICKROSS=testrouter
router: test-build-$(OS)
router: 	
	@printf ''>/dev/null


# Chain test program
chains: RICKROSS=testchains
chains: test-build-$(OS)
chains: 
	@printf ''>/dev/null

# Render test program
render: RICKROSS=testrender
render: test-build-$(OS)
render: 
	@printf ''>/dev/null

# All test build programs use this recipe
# But notice that a version exists for different operating systems. 
# OSX
test-build-Darwin: $(OBJ)
test-build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua

test-build-CYGWIN: $(OBJ)
test-build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua

# Linux
# These systems may need these additional commands: 
# $(shell pkg-config --libs lua5.3)
# $(shell pkg-config --cflags lua5.3)
test-build-Linux: $(OBJ) 
test-build-Linux:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua -ldl -lpthread -lm
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua -ldl -lpthread -lm

	
# A not-so-main target, that will probably result in a few object files...
objs: $(OBJ)
	@printf "Finished building objects\n"


# An update target?  (checks for new versions of all your vendor dependencies...)	
update:
	@printf "Not yet finished."


# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) testrouter testchains
	-@find . | egrep '\.o$$' | xargs rm

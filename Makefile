# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
CLANGFLAGS = -g -O0 -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H -DNW_PERFLOG
CC = clang
CFLAGS = $(CLANGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O0 -DNW_DEBUG -DNW_PERFLOG -Wno-format-security #-ansi
CC = gcc
CFLAGS = $(GCCFLAGS)
PORT = 2200

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

# CLI
cli: RICKROSS=$(NAME)b
cli: test-build-$(OS)
cli: 	
	@printf ''>/dev/null

# Add and test two localhost names
add-hosts:
	@printf "127.0.0.1\tkhan.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.khan.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\tability.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.ability.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\terrors.com #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.errors.com #added by hypno\n" >> /etc/hosts

# Remove hosts
remove-hosts:
	@sed -i '/#added by hypno/d' /etc/hosts

# Make an SSL Client
tlscli:
	$(CC) -DSQROOGE_H $(CFLAGS) -o cx vendor/single.c tlscli.c -lgnutls

# Make an SSL server that just runs forever... 
tlssvr:
	$(CC) -DSQROOGE_H $(CFLAGS) -o sx vendor/single.c tlssvr.c -lgnutls

# Build a client with axtls
axtlscli:
	$(CC) -DSQROOGE_H $(CFLAGS) -o cxax vendor/single.c tlscli-axtls.c -laxtls

# All test build programs use this recipe
# But notice that a version exists for different operating systems. 
# OSX
test-build-Darwin: $(OBJ)
test-build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(RICKROSS) -llua

# Cygwin / Windows
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

# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) hypnob testrouter testchains testsql testrender
	-@find . | egrep '\.o$$' | grep -v sqlite | xargs rm

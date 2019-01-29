# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
LDFLAGS = -Llib/hypno -Iinclude
CLANGFLAGS = -g -O0 -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H -DNW_PERFLOG
CC = clang
CFLAGS = $(CLANGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -Wno-format-security -O0 $(LDFLAGS) -DNW_DEBUG -DNW_PERFLOG
CC = gcc
CFLAGS = $(GCCFLAGS)
PREFIX = /usr/local
VERSION = 0.01

# Some Linux systems need these, but pkg-config should handle it
#INCLUDE_DIR=-I/usr/include/lua5.3
#LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c
OBJ = ${SRC:.c=.o}

# A main target, that will most likely result in a binary
main: BINNAME=main
main: build-$(OS)
main: 
	mv $(BINNAME) bin/$(NAME)


# Install
install:
	cp $(NAME) $(PREFIX)/bin/
	[ -f $(NAME)-admin ] && cp $(NAME)-admin $(PREFIX)/bin/


# Build all the tests
test:
	printf '' >/dev/null


# CLI
cli: BINNAME=$(NAME)-admin
cli: build-$(OS)
cli: 	
	mv $(BINNAME) bin/


# Router test program
router: BINNAME=testrouter
router: build-$(OS)
router: 	
	@printf ''>/dev/null


# Chain test program
chains: BINNAME=testchains
chains: build-$(OS)
chains: 
	@printf ''>/dev/null


# SQL test program
sql: BINNAME=testsql
sql: build-$(OS)
sql: 
	@printf ''>/dev/null


# Render test program
render: BINNAME=testrender
render: build-$(OS)
render: 
	@printf ''>/dev/null


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
build-Darwin: $(OBJ)
build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua


# Cygwin / Windows
build-CYGWIN: $(OBJ)
build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua


# Linux
build-Linux: $(OBJ) 
build-Linux:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua -ldl -lpthread -lm 
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua -ldl -lpthread -lm


# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) $(NAME)-admin test{chains,render,router,sql} $(NAME)-$(VERSION).tar.gz 2>/dev/null
	-@rm -rf $(NAME)-$(VERSION) *dSYM 2>/dev/null
	-@find . | egrep '\.o$$' | grep -v sqlite | xargs rm 2>/dev/null


# Make a package for distribution
pkg: clean
pkg:
	git clone . $(NAME)-$(VERSION)
	make gitlog > $(NAME)-$(VERSION)/changelog.md
	markdown $(NAME)-$(VERSION)/changelog.md > $(NAME)-$(VERSION)/changelog.html && rm $(NAME)-$(VERSION)/changelog.md
	rm -rf $(NAME)-$(VERSION)/tests/
	tar czf $(NAME)-$(VERSION).tar.gz $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION)/
			

# gitlog - Generate a full changelog from the commit history
gitlog:
	@printf "# CHANGELOG\n\n"
	@printf "## STATS\n\n"
	@printf -- "- Commit count: "
	@git log --full-history --oneline | wc -l
	@printf -- "- Project Inception "
	@git log --full-history | grep Date: | tail -n 1
	@printf -- "- Last Commit "
	@git log -n 1 | grep Date:
	@printf -- "- Authors:\n"
	@git log --full-history | grep Author: | sort | uniq | sed '{ s/Author: //; s/^/\t- /; }'
	@printf "\n"
	@printf "## HISTORY\n\n"
	@git log --full-history --author=Antonio | sed '{ s/^   /- /; }'
	@printf "\n<link rel=stylesheet href=changelog.css>\n"

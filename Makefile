## --------------------------------------------------------- ##
## Makefile
## 
## @summary
## Contains all the targets necessary to assemble
## Hypno on most machines. 
##
## Eventually, this will be dropped in favor of
## autotools.
##
## @author
## Copyright 2020 Tubular Modular Inc. dba Collins Design
## --------------------------------------------------------- ##
NAME = hypno
USER = http
GROUP = http
OS = $(shell uname | sed 's/[_ ].*//')
LDFLAGS = -lgnutls -llua -ldl -lpthread -lsqlite3
DEBUGFLAGS = -DSKIPMYSQL_H -DSKIPPGSQL_H -DDEBUG_H
CLANGFLAGS = -g -O0 -Wall -Werror -Wno-unused -Wno-format-security \
	-fsanitize=address -fsanitize-undefined-trap-on-error $(DEBUGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -Wno-strict-aliasing \
	-Wno-format-truncation -Wno-strict-overflow -Wno-deprecated-declarations \
	-Wno-return-local-addr -O2 $(DEBUGFLAGS) 
CFLAGS = $(CLANGFLAGS)
#CFLAGS = $(GCCFLAGS)
CC = clang
#CC = gcc
PREFIX = /usr/local
VERSION = 0.3
TESTS = config loader database luabind router util #server render filter
SRC = vendor/zrender.c vendor/zhasher.c vendor/zwalker.c vendor/zhttp.c \
	src/config.c src/hosts.c src/db-sqlite.c src/luabind.c src/mime.c \
	src/socket.c src/util.c src/ctx-http.c src/ctx-https.c src/server.c \
	src/loader.c src/mvc.c src/filter-static.c src/filter-lua.c \
	src/router.c #src/filter-echo.c src/filter-dirent.c src/filter-c.c src/xml.c src/json.c src/dirent-filter.c
OBJ = ${SRC:.c=.o}

# main - Compiles all code needed to get hypno running
main: $(OBJ)
	-@test -d bin/ || mkdir bin/
	$(CC) $(LDFLAGS) $(CFLAGS) src/main.c -o bin/$(NAME) $(OBJ) 
	$(CC) $(LDFLAGS) $(CFLAGS) src/cli.c -o bin/hcli $(OBJ)

# install - Installs targets to $PREFIX/bin
install:
	cp bin/hypno bin/hcli $(PREFIX)/bin/

# examples - Runs hypno with the files in example/.  Use -e PORT to change port number.
examples: PORT=2222
examples: 
	bin/hypno --start --port $(PORT) --config example/config.lua 

# repl - Build and test libraries for REPL usage
#	test -f sqlite3.o || $(CC) $(CFLAGS) -fPIC -c vendor/sqlite3.c -o shared/sqlite3.o
repl:
	$(CC) $(CFLAGS) -fPIC -c src/database.c -o shared/database.o
	$(CC) $(CFLAGS) -fPIC -c vendor/zhasher.c -o shared/zhasher.o
	$(CC) $(CFLAGS) -fPIC -c src/luabind.c -o shared/luabind.o
	$(CC) $(CFLAGS) -fPIC -c src/lua-db.c -o shared/lua-db.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS) -fPIC -o lib$(NAME).so shared/database.o \
		shared/lua-db.o shared/zhasher.o shared/luabind.o
	lua -l libhypno - < tests/lua-db/dbtest.lua
 
%.o: %.c 
ifeq ($(OS),CYGWIN)
	$(CC) -c -o $@ $< $(CFLAGS)
else
	$(CC) -c -o $@ $< $(CFLAGS)
endif

# test -  Build all test files
test: $(OBJ) 
test: CFLAGS+=-DTEST_H
test:
	-@test -d bin/ || mkdir bin/
	for t in $(TESTS); do $(CC) $(LDFLAGS) $(CFLAGS) -o bin/$$t src/test/$${t}-test.c $(OBJ); done

# deps - Build some of the larger dependencies first (they take a minute)
deps:
	$(CC) $(CFLAGS) -o vendor/sqlite3.o -c vendor/sqlite3.c 

# clean - Get rid of object files and tests 
clean:
	-@find src/ -maxdepth 1 -type f -name "*.o" | xargs rm
	-@find bin/ -maxdepth 1 -type f | xargs rm
	-@find vendor/ -maxdepth 1 -type f -name "*.o" ! -name "sqlite3.o" | xargs rm
	-@rm hcli hypno
	-@find -maxdepth 1 -type f -name "vgcore*" | xargs rm

# list - List all the targets and what they do
list:
	@printf 'Available options are:\n'
	@sed -n '/^# / { s/# //; 1d; p; }' Makefile | awk -F '-' '{ printf "  %-20s - %s\n", $$1, $$2 }'

#if 0
# pkg - Make a package of the latest tagged version for distribution
pkg:
	git archive --format tar HEAD `git tag | tail -n 1` | \
		gzip > $(NAME)-`git tag | tail -n 1`.tar.gz

# pkgtest - Make a package of the latest version of dev
pkgtest:
	git archive --format tar HEAD | \
		gzip > $(NAME)-`git tag | tail -n 1`.tar.gz

# makefile - Generate a Makefile appropriate for a regular user
makefile:
	@sed '/^# /d' Makefile | cpp - | sed '/^# /d'

# changelog - Generate a full changelog from the commit history
changelog:
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
#endif

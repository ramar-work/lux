# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
LDFLAGS = -Llib/hypno -Iinclude
CLANGFLAGS = -g -O0 -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H -DNW_PERFLOG
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -Wno-strict-aliasing -std=c99 -Wno-deprecated-declarations -O2 -Wno-format-truncation $(LDFLAGS) -DDEBUG_H #-ansi
CFLAGS = $(CLANGFLAGS)
CFLAGS = $(GCCFLAGS)
CC = clang
CC = gcc
PREFIX = /usr/local
VERSION = 0.01
PORT = 2200
RANDOM_PORT = 1
PORT_FILE = /tmp/hypno.port
BROWSER = chromium
RECORDS=3

# Some Linux systems need these, but pkg-config should handle it
#INCLUDE_DIR=-I/usr/include/lua5.3
#LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
#SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c socketmgr.c bridge.c 
SRC = vendor/single.c vendor/sqlite3.c socketmgr.c #bridge.c 
OBJ = ${SRC:.c=.o}


# Unfortunately, I'm still working on this...
default:
	$(CC) $(CFLAGS) -lgnutls -DSQROOGE_H -o bin/socketmgr socketmgr.c vendor/single.c

clang:
	clang $(CLANGFLAGS) -lgnutls -DSQROOGE_H -o bin/socketmgr socketmgr.c vendor/single.c

gcc:
	gcc $(GCCFLAGS) -lgnutls -DSQROOGE_H -o bin/socketmgr socketmgr.c vendor/single.c

# A main target, that will most likely result in a binary
main: BINNAME=main
main: FILENAME=&1
main: build-$(OS)
main: 
	mv $(RICKROSS) bin/hypno
	$(CC) $(CFLAGS) -lgnutls -DSQROOGE_H -o th test.c vendor/single.c && mv th bin/

# Run a test on the server running in the foreground
test-srv:
	bin/socketmgr --start --port $(PORT) 

# Run a test on the server running in the foreground, using LLVM
test-srv-asan:
	-@make kill-srv
	make clean
	make clang
	test $(RANDOM_PORT) -eq 1 && MYPORT=$$(( ( $$RANDOM % 7000 ) + 1000 )) || MYPORT=$(PORT); \
	echo $$MYPORT > $(PORT_FILE); \
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) bin/socketmgr --start --port $$MYPORT

# Run a test on the server running in the foreground, using Valgrind 
test-srv-vg:
	-@make kill-srv
	make clean
	make gcc 
	test $(RANDOM_PORT) -eq 1 && MYPORT=$$(( ( $$RANDOM % 7000 ) + 1000 )) || MYPORT=$(PORT); \
	echo $$MYPORT > $(PORT_FILE); \
	valgrind bin/socketmgr --start --port $$MYPORT;

# Run a test on the server running in the foreground
kill-srv:
	ps aux | grep bin/socketmgr | awk '{ print $$2 }' | xargs kill -9

# Run a test with a variety of clients
test-cli:
	@test -f $(PORT_FILE) && MYPORT=`cat $(PORT_FILE)` || MYPORT=$(PORT); \
	HTMLDOC=/tmp/hypno-test-run.html; \
	tests/test.sh --keep -v --port $$MYPORT > $$HTMLDOC && \
		$(BROWSER) file://$$HTMLDOC || echo "Test suite failed..."

		
test-cli-nobrowser:
	@test -f $(PORT_FILE) && MYPORT=`cat $(PORT_FILE)` || MYPORT=$(PORT); \
	tests/test.sh -v --keep --port $$MYPORT 


# init-test - Generate test data for use by different web clients
init-test:
	@echo '(WARNING: This could take a while.)'
	@tests/test.sh -v --init-tests -r $(RECORDS) 

# Test suites can be randomly generated
gen-test:
	sqlite3 tests/test.db "select curl_headers,curl_body,url,get from t where uuid = 1;" | \
		awk \
			-vSITE="http://localhost:2000" \
			-F '|' '{
				printf "curl %s %s %s%s%s\n", $1, $2, SITE, $3, $4
		}'	

# How do I put this together?
exec:
	sqlite3 tests/test.db

#sqlite3 tests/test.db < tests/test.sql;
# CLI
cli: RICKROSS=cli
cli: test-build-$(OS)
cli: 	
	mv $(BINNAME) bin/

# Build all tests and the server + scripting backend
all:
	$(MAKE)
	$(MAKE) agg
	$(MAKE) chains 
	$(MAKE) render
	$(MAKE) router 
	$(MAKE) sql 

# Table dump test program
ldump: RICKROSS=ldump
ldump: test-build-$(OS)
ldump: 	
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
build-Darwin: $(OBJ)
build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua 2>$(FILENAME)


# Cygwin / Windows
build-CYGWIN: $(OBJ)
build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua 2>$(FILENAME)

# test-build-Linux 
test-build-Linux: $(OBJ) 
test-build-Linux:
	@echo $(CC) $(CFLAGS) $(OBJ) -o $(RICKROSS) -llua -ldl -lpthread -lm 
	@$(CC) $(CFLAGS) $(OBJ) -o $(RICKROSS) -llua -ldl -lpthread -lm

# clean - Get rid of the crap
clean:
	-@rm $(NAME) cli testrouter testchains testsql testrender bin/*
	-@find . | egrep '\.o$$' | grep -v sqlite | xargs rm

# pkg - Make a package for distribution
pkg:
	git archive --format tar HEAD | gzip > $(NAME)-$(VERSION).tar.gz

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

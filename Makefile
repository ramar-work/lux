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

# Use this flag before invoking programs, leave it blank on some systems
#INVOKE = ASAN_SYMBOLIZER_PATH=$$(locate llvm-symbolizer | grep "llvm-symbolizer$$")
INVOKE = valgrind 

# Some Linux systems need these, but pkg-config should handle it
#INCLUDE_DIR=-I/usr/include/lua5.3
#LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c
OBJ = ${SRC:.c=.o}

# A main target, that will most likely result in a binary
main: BINNAME=main
main: FILENAME=&1
main: build-$(OS)
main: 
	mv $(BINNAME) bin/$(NAME)

# Silent
silent: BINNAME=main
silent: FILENAME=/tmp/err
silent: build-$(OS)
silent: 
	mv $(BINNAME) bin/$(NAME)

# zmq w/ czmq
czmq:
	gcc -g -Wall -Werror -Wno-unused -Wstrict-overflow \
	-Wno-deprecated-declarations -Wno-format-security -O0 -DNW_DEBUG -DNW_PERFLOG \
		zmqss.c -Iinclude -lczmq -o zy

# zmq
zmq: 
	gcc -g -Wall -Werror -Wno-unused -Wstrict-overflow \
	-Wno-deprecated-declarations -Wno-format-security -O0 -DNW_DEBUG -DNW_PERFLOG \
		zmqc.c -Iinclude -L. -lzmq -o zc 
	gcc -g -Wall -Werror -Wno-unused -Wstrict-overflow \
	-Wno-deprecated-declarations -Wno-format-security -O0 -DNW_DEBUG -DNW_PERFLOG \
		zmqs.c -Iinclude -L. -lzmq -o zs
	gcc -g -Wall -Werror -Wno-unused -Wstrict-overflow \
	-Wno-deprecated-declarations -Wno-format-security -O0 -DNW_DEBUG -DNW_PERFLOG \
		zmq.c -Iinclude -L. -lzmq -o zz 
	
#$(CC) $(CFLAGS) zmq.c -lzmq -o zz 


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

# SQL test program and routines to populate the test's required database
sql: RICKROSS=tests/sql
sql: test-build-$(OS)
sql: 
	@echo "Generating test data for SQL parse tests (WARNING: This could take a while....)"
	@sed -n 2,14p tests/sql-data/ges.txt > tests/sql-data/ges.create.sql
	@sqlite3 tests/sql-data/ges.db < tests/sql-data/ges.create.sql
	@sed -n 20,118p tests/sql-data/ges.txt > tests/sql-data/ges.import.sql
	@sqlite3 -separator ',' tests/sql-data/ges.db ".import tests/sql-data/ges.import.sql general_election"
	@sqlite3 tests/sql-data/ges.db 'select * from general_election limit 10' 
	@-rm tests/sql-data/ges.create.sql tests/sql-data/ges.import.sql
	@echo "DONE!"

# Render test program
render: RICKROSS=tests/render
render: test-build-$(OS)
render: 
	@printf ''>/dev/null

# Aggregation test program
agg: RICKROSS=tests/agg
agg: test-build-$(OS)
agg: 
	@printf ''>/dev/null

# Depth and stackdump test program
depth: RICKROSS=tests/depth
depth: test-build-$(OS)
depth: 
	@echo "Generating test data for depth tests (WARNING: This could take a while....)"
	@for NUM in `seq 1 5`; do tests/scripts/dd.sh > tests/depth-data/dd$${NUM}.test && lua tests/depth-data/dd$${NUM}.test; done
	@echo "DONE!"

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
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua 2>$(FILENAME)


# Cygwin / Windows
build-CYGWIN: $(OBJ)
build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua 2>$(FILENAME)

# Linux
build-Linux: $(OBJ) 
build-Linux:
	@echo $(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua -ldl -lpthread -lm 
	@$(CC) $(CFLAGS) $(OBJ) $(BINNAME).c -o $(BINNAME) -llua -ldl -lpthread -lm 2>$(FILENAME)


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

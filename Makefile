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
PREFIX = /usr/local
VERSION = 0.01
WWWROOT = $(HOME)/prj/$(NAME)-wwwroot
HOSTS = hypno.local hypno-test.local

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
	mv $(RICKROSS) $(NAME) 


# Install
install:
	cp $(NAME) $(PREFIX)/bin/
	[ -f $(NAME)-admin ] && cp $(NAME)-admin $(PREFIX)/bin/


# CLI
cli: RICKROSS=$(NAME)-admin
cli: test-build-$(OS)
cli: 	
	@printf ''>/dev/null

#Objects
#.c.o:
#	@echo $(CC) $(CFLAGS) -c $<
#	@$(CC) $(CFLAGS) -c $<

# Kill a running server
kill:
	ps aux | grep './$(NAME) --no-daemon --start' | awk '{ print $$2 }' | xargs kill -9 

# Make a request
req:
	curl http://billtracker.local:$(PORT)/home > tests/home.html

#
ssl:
	./$(NAME) --no-daemon --start --port 443 --dir $(WWWROOT)

vchim:
	valgrind ./$(NAME) --no-daemon --start --port $(PORT) --dir $(WWWROOT)

chim:
	./$(NAME) --no-daemon --start --port $(PORT) --dir $(WWWROOT)

# This tests how $(NAME) starts with a certain set of criteria.
goku:
	./$(NAME) --no-daemon --start --port $(PORT) 

# This tests $(NAME) and logs to a file named baby.log
baby:
	./$(NAME) --no-daemon --start --port $(PORT) 2>baby.log

# This is a test request to use with Curl or Wget	
gohan:
	wget -qO /tmp/index.html http://localhost:$(PORT)/multi

# This is a test request using a hostname to test hypno's virtual hosting capability
khan:
	@test `grep -c khan.org /etc/hosts` -gt 0 && wget -O /tmp/index.html http://khan.org:$(PORT)/multi || echo "khan.org not found in /etc/hosts.  Run 'make hosts' to add it and others."

# This is a test request to use with Curl or Wget	
gotenks:
	wget -O /tmp/index.html http://localhost:$(PORT)

# This is a test request that grabs files full of errors
errors:
	@test `grep -c errors.com /etc/hosts` -gt 0 && wget -O /tmp/index.html http://errors.com:$(PORT)/failure || echo "errors.com not found in /etc/hosts.  Run 'make hosts' to add it and others."

# Add and test two localhost names
add-hosts:
	@for n in $(HOSTS); do printf "127.0.0.1\t$$n # added by hypno\n"; done >> /etc/hosts

# Remove hosts
remove-hosts:
	@sed -i '/#added by hypno/d' /etc/hosts

# A top-level target that builds everything
test:
	make router; \
	make chains; \
	make render; \
	make sql; \
	make main

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


# SQL test program
sql: RICKROSS=testsql
sql: test-build-$(OS)
sql: 
	@printf ''>/dev/null


# Render test program
render: RICKROSS=testrender
render: test-build-$(OS)
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

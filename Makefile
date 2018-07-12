# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O0 #-DDEBUG_H #-ansi
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

#Objects
#.c.o:
#	@echo $(CC) $(CFLAGS) -c $<
#	@$(CC) $(CFLAGS) -c $<

# This tests how hypno starts with a certain set of criteria.
goku:
	./hypno --no-daemon --start --port $(PORT) 

# This tests hypno and logs to a file named baby.log
baby:
	./hypno --no-daemon --start --port $(PORT) 2>baby.log

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
	@printf "127.0.0.1\tkhan.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.khan.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\tability.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.ability.org #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\terrors.com #added by hypno\n" >> /etc/hosts
	@printf "127.0.0.1\twww.errors.com #added by hypno\n" >> /etc/hosts

# Remove hosts
remove-hosts:
	@sed -i '/#added by hypno/d' /etc/hosts

# A top-level target that builds everything
top:
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

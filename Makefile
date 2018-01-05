# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O0 -DDEBUG_H #-ansi
CC = gcc
CFLAGS = $(GCCFLAGS)
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)

# Some Linux systems need these, but pkg-config should handle it
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c
OBJ = ${SRC:.c=.o}


# ...
will:
	make agg && ./agg

# A main target, that will most likely result in a binary
main: RICKROSS=main
main: test-build-$(OS)
main: 
	mv $(RICKROSS) hypno


# Router test program
router: RICKROSS=tests/router
router: test-build-$(OS)
router: 	
	@printf ''>/dev/null


# Chain test program
chains: RICKROSS=tests/chains
chains: test-build-$(OS)
chains: 
	@printf ''>/dev/null

# SQL test program and routines to populate the test's required database
sql: RICKROSS=tests/sql
sql: test-build-$(OS)
sql: 
	@sed -n 2,14p tests/sql-data/ges.txt > tests/sql-data/ges.create.sql
	@sqlite3 tests/sql-data/ges.db < tests/sql-data/ges.create.sql
	@sed -n 20,118p tests/sql-data/ges.txt > tests/sql-data/ges.import.sql
	@sqlite3 -separator ',' tests/sql-data/ges.db ".import tests/sql-data/ges.import.sql general_election"
	@sqlite3 tests/sql-data/ges.db 'select * from general_election limit 10' 
	@-rm tests/sql-data/ges.create.sql tests/sql-data/ges.import.sql

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

# All test build programs use this recipe
# But notice that a version exists for different operating systems. 
# OSX
test-build-Darwin: $(OBJ)
test-build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua

test-build-CYGWIN: $(OBJ)
test-build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua

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
	-@rm $(NAME) agg router chains sql render
	-@find . | egrep '\.o$$' | grep -v 'sqlite3.o' | xargs rm

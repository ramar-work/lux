# This project...
NAME = hypno
OS = $(shell uname | sed 's/[_ ].*//')
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang
CFLAGS = $(CLANGFLAGS)
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 -Wno-deprecated-declarations -O2 -DDEBUG_H #-ansi
CC = gcc
CFLAGS = $(GCCFLAGS)

# Use this flag before invoking programs, leave it blank on some systems
#INVOKE = ASAN_SYMBOLIZER_PATH=$$(locate llvm-symbolizer | grep "llvm-symbolizer$$")
INVOKE = valgrind 

# Some Linux systems need these, but pkg-config should handle it
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c #bridge.c
OBJ = ${SRC:.c=.o}


# ...
will:
	make agg && $(INVOKE) ./agg

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

# Depth and stackdump test program
depth: RICKROSS=tests/depth
depth: test-build-$(OS)
depth: 
	@for NUM in `seq 1 5`; do scripts/dd.sh > tests/depth-data/dd-$${NUM}.test && lua tests/depth-data/dd-$${NUM}.test; done

# All test build programs use this recipe
# But notice that a version exists for different operating systems. 
# OSX
test-build-Darwin: $(OBJ)
test-build-Darwin: bridge.o 
test-build-Darwin:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua

# Cygwin
test-build-CYGWIN: $(OBJ)
test-build-CYGWIN: bridge.o
test-build-CYGWIN:
	@echo $(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua
	@$(CC) $(CFLAGS) $(OBJ) $(RICKROSS).c -o $(shell basename $(RICKROSS)) -llua

# Linux
test-build-Linux: $(OBJ) 
test-build-Linux: LUA_V=5.2
test-build-Linux:
	@echo $(CC) $(CFLAGS) $(shell pkg-config --cflags lua$(LUA_V)) -c bridge.c
	@$(CC) $(CFLAGS) $(shell pkg-config --cflags lua$(LUA_V)) -c bridge.c && echo "DONE!" || echo "Failed to compile bridge.c..."
	@echo $(CC) $(CFLAGS) $(shell pkg-config --cflags lua$(LUA_V)) $(OBJ) bridge.o $(RICKROSS).c -o $(shell basename $(RICKROSS)) $(shell pkg-config --libs lua$(LUA_V)) -ldl -lpthread -lm
	@$(CC) $(CFLAGS) $(shell pkg-config --cflags lua$(LUA_V)) $(OBJ) bridge.o $(RICKROSS).c -o $(shell basename $(RICKROSS)) $(shell pkg-config --libs lua$(LUA_V)) -ldl -lpthread -lm

	
# A not-so-main target, that will probably result in a few object files...
objs: $(OBJ)
	@printf "Finished building objects\n"


# An update target?  (checks for new versions of all your vendor dependencies...)	
update:
	@printf "Not yet finished."


# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) agg router chains sql render depth
	-@find . | egrep '\.o$$' | grep -v 'sqlite3.o' | xargs rm


# Echo (for debugging)
echo:
	@printf "%-10s => %s\n" "CC" $(CC) 
	@printf "%-10s => %s\n" "OS" $(OS) 
	@printf "%-10s => %s\n" "CFLAGS" "$(CFLAGS)" 
	@printf "%-10s => %s\n" "OBJ" "$(OBJ)" 
	@printf "%-10s => %s\n" "IGNCLEAN" $(IGNCLEAN) 
	@printf "%-10s => %s\n" "LUA_V" $(LUA_V) 

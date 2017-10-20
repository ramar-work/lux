# This project...
NAME = hypno
SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c bridge.c main.c
OBJ = ${SRC:.c=.o}

GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O0 -DDEBUG_H
CC = gcc

CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize-undefined-trap-on-error -Wno-format-security -DDEBUG_H
CC = clang

CFLAGS = $(CLANGFLAGS)


# A main target, that will most likely result in a binary
main: $(OBJ)
	@echo $(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -llua


# A not-so-main target, that will probably result in a few object files...
objs: $(OBJ)
	@printf "Finished building objects\n"


# A packaging target
pkg:


# An update target?  (checks for new versions of all your vendor dependencies...)	


# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@find . | egrep '\.o$$' | xargs rm
#		sed '/sqlite3.o/d' #| xargs rm 
#	-@rm $(BINS) vendor/sqlite3.o 2>/dev/null

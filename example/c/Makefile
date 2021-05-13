NAME=app

# Eventually we will need to use sanitization
DFLAGS = -g -O0 -Wall -Werror -Wno-unused -Wno-format-security \
	-fsanitize=address -fsanitize-undefined-trap-on-error \
	-fPIC -std=c99 -I../../vendor

# This works for this
CFLAGS = -g -O0 -Wall -Werror -Wno-unused -Wno-format-security \
	-fPIC -std=c99 -I../../vendor

# This works for the actual library
BFLAGS = -g -O0 -Wall -Werror -Wno-unused -Wno-format-security \
	-std=c99 -I../../vendor

CC=clang

main: vendor/zhttp.o vendor/zwalker.o
	$(CC) $(BFLAGS) -shared -o app.so vendor/zhttp.o vendor/zwalker.o

clean:
	rm -f *.o vendor/*.o

dltest:
	$(CC) -Wall -Werror -ldl -o dylib dylib.c

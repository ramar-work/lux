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
RECORDS=10

# Some Linux systems need these, but pkg-config should handle it
INCLUDE_DIR=-I/usr/include/lua5.3
LD_DIRS=-L/usr/lib/x86_64-linux-gnu

# Not sure why these don't always work...
#SRC = vendor/single.c vendor/nw.c vendor/http.c vendor/sqlite3.c socketmgr.c bridge.c 
SRC = vendor/single.c vendor/sqlite3.c socketmgr.c #bridge.c 
OBJ = ${SRC:.c=.o}


# Unfortunately, I'm still working on this...
a:
	$(CC) $(CFLAGS) -lgnutls -DSQROOGE_H -o bin/socketmgr socketmgr.c vendor/single.c
	$(CC) $(CFLAGS) -lgnutls -DSQROOGE_H -o bin/th test.c vendor/single.c

# A main target, that will most likely result in a binary
main: RICKROSS=main
main: test-build-$(OS)
main: 
	mv $(RICKROSS) bin/hypno
	$(CC) $(CFLAGS) -lgnutls -DSQROOGE_H -o th test.c vendor/single.c && mv th bin/


# Generate a bunch of stuff
# You can have a file called test.sh with options...


# You can generate a PPM file as a quick easy test of binary image formats...
ppm-test:
	printf "P6\n%d %d\n255\n" 20 20 > /tmp/my.ppm
	head -c $$(( 20 * 20 )) /dev/urandom >> /tmp/my.ppm


# generate a bunch of random addresses
# - random addresses
# - random numbers 
# - random names 
# - random blocks of text (from 1 - 30mb or more) 
# - random binary data 
init-test:
	@echo "Initializing test suite (this could take a while...)"
	@echo "CREATE TABLE t ( \
		uuid INTEGER PRIMARY KEY AUTOINCREMENT, \
		id TEXT, \
		url TEXT, \
		curl_headers TEXT, \
		wget_headers TEXT, \
		chrome_headers TEXT, \
		get TEXT, \
		curl_body TEXT, \
		wget_body TEXT, \
		chrome_body TEXT \
	);\
	" > tests/test.sql; \
	for i in `seq 0 $(RECORDS)`; do \
		WL=`wc -l tests/words | awk '{ print $$1 }'`; \
		BL=`wc -l tests/wordblocks | awk '{ print $$1 }'`; \
		WNOL=`wc -l tests/words_no_apostrophe | awk '{ print $$1 }'`;  \
		ALPHABET="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" && \
		RANDSTR= && \
		for n in `seq 0 32`; do \
			RANDSTR+=$${ALPHABET:$$(( $$RANDOM % $${#ALPHABET} )):1}; \
		done; \
		HEADER= && CURL_HEADER= && WGET_HEADER= && CHROME_HEADER= && HEADER_ARR= && \
		for n in `seq 0 $$(( $$RANDOM % 20 ))`; do \
			HEADERBLOCK="X-header-xxxx-`sed -n "$$(( $$RANDOM % $$WNOL ))p" tests/words_no_apostrophe`: `sed -n "$$(( $$RANDOM % $$WNOL ))p" tests/words_no_apostrophe`"; \
			HEADER+="$$HEADERBLOCK"; \
			CURL_HEADER+=" -H '$$HEADERBLOCK'"; \
			WGET_HEADER+=" --header '$$HEADERBLOCK'"; \
			CHROME_HEADER+="$$HEADERBLOCK"; \
			HEADER_ARR[ n ]="$$HEADERBLOCK<br>"; \
		done; \
		GET= && GET_ARR= && \
		for n in `seq 0 $$(( $$RANDOM % 20 ))`; do \
			GETBLOCK="&`sed -n "$$(( $$RANDOM % $$WNOL ))p" tests/words_no_apostrophe`=`sed -n "$$(( $$RANDOM % $$BL ))p" tests/wordblocks`"; \
			GET+="$$GETBLOCK" \
			GET_ARR[ n ]="$$GETBLOCK<br>"; \
		done; \
		BODY= && CURL_BODY= && WGET_BODY= && CHROME_BODY= && BODY_ARR= && \
		for n in `seq 0 $$(( $$RANDOM % 13 ))`; do \
			BODYBLOCK="`sed -n "$$(( $$RANDOM % $$BL ))p" tests/wordblocks`"; \
			INNERKEY= ; \
			for j in `seq 0 $$(( $$RANDOM % 37 ))`; do \
				INNERKEY+=$${ALPHABET:$$(( $$RANDOM % $${#ALPHABET} )):1}; \
			done; \
			BODY+="$$BODYBLOCK"; \
			CURL_BODY+=" --data-urlencode '$$INNERKEY=$$BODYBLOCK'"; \
			WGET_BODY+="&$$BODYBLOCK"; \
			CHROME_BODY+="$$BODYBLOCK"; \
			BODY_ARR[ n ]="$$INNERKEY=$$BODYBLOCK<br>\n"; \
		done; \
		URLBODY= && URLBODY_ARR= && \
		for n in `seq 0 $$(( $$RANDOM % 13 ))`; do \
			URLBLOCK="/`sed -n "$$(( $$RANDOM % $$WNOL ))p" tests/words_no_apostrophe`"; \
			URLBODY+="$$URLBLOCK"; \
			URLBODY_ARR[ n ]="$$URLBLOCK"; \
		done; \
		test -f /tmp/shimmy && rm /tmp/shimmy || printf ''; \
		printf "<h2>URL</h2>\n$$URLBODY<br>\n" >> /tmp/shimmy; \
		printf "\n<h2>Headers</h2>" >> /tmp/shimmy;  \
		for n in $${HEADER_ARR[@]}; do printf "$$n" | sed 's/:/ => /; s/X-/\nX-/g'; done >> /tmp/shimmy; \
		printf "\n\n<h2>GET</h2>" >> /tmp/shimmy; \
		for n in $${GET_ARR[@]}; do printf "$$n " | sed 's/=/ => /; s/\&/\n/g'; done >> /tmp/shimmy; \
		printf "\n\n<h2>POST</h2>\n" >> /tmp/shimmy;  \
		for n in $${BODY_ARR[@]}; do printf "$$n " | sed 's/=/ => /'; done >> /tmp/shimmy; \
		OIFS="$$IFS"; IFS="^"; IFS="$$OIFS"; \
		printf "INSERT INTO t VALUES( \
			 NULL, \
			'$$RANDSTR', \
			'$$URLBODY', \
			'`printf -- "$$CURL_HEADER" | sed "s/'/''/g"`', \
			'`printf -- "$$WGET_HEADER" | sed "s/'/''/g"`', \
			'`printf -- "$$CHROME_HEADER" | sed "s/'/''/g"`', \
			'`printf -- "$$GET" | sed "s/^./?/; s/ /%%20/g"`', \
			'`printf -- "$$CURL_BODY" | sed "s/'/''/g"`', \
			'`printf -- "$$WGET_BODY" | sed "s/'/''/g"`', \
			'`printf -- "$$CHROME_BODY" | sed "s/'/''/g"`' \
		 );\n" >> tests/test.sql; \
	done; 
	@test -f tests/test.db && rm tests/test.db || printf ''>/dev/null;
	@sqlite3 tests/test.db < tests/test.sql;
	@echo "DONE!."


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
	@echo $(CC) $(CFLAGS) $(OBJ) -o $(RICKROSS) -llua -ldl -lpthread -lm 
	@$(CC) $(CFLAGS) $(OBJ) -o $(RICKROSS) -llua -ldl -lpthread -lm

# Clean target...
#		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` 
clean:
	-@rm $(NAME) cli testrouter testchains testsql testrender
	-@find . | egrep '\.o$$' | grep -v sqlite | xargs rm

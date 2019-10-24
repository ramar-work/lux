#!/bin/bash -
# -----------------------------------------------------------------------------  
# test.sh
# =======
# 
# This is a script for stress testing HTTP servers and handlers.  The most solid 
# way to run this is against an echo server.  It just for success and a matching
# response.
#
#
# Usage
# -----
# TBD
#
#
# Notes
# -----
# Test parameters:
# 
# - This assumes that neither CONNECT, TRACE, nor OPTIONS methods are supported.
# - `make init-test` has already been run. (which can optionally be put here...)
#
# Questions this should answer:
#
# - Was the request successful (or was there some kind of crash?)
# - Was invalid input processed?
#
# 
# TODO
# ----
# - Move `make init-test` here.  Call it with option --initialize or --init
#
#
# -----------------------------------------------------------------------------  
THIS=hypnotest
CONST_DB=126
CONST_FILE=127
OUTPUT_TYPE=$CONST_FILE
CLIENTS=("wget" "curl")
#CLIENTS=("wget" "curl" "chromium" "opera")
CLIENT=
SERVER=bin/hypno
WWWADDR=http://localhost
WWWPORT=2000

THREAD=0
OUTPUT_DIR=/tmp/hypno-tests

URLFILE=
RANDOMIZE_CLIENTS=0
WAIT_FOR_INPUT=0
DRYRUN=0
VERBOSE=0
SLEEP=0
INITTEST=0
SLEEP_INTERVAL=5
RECORDS=3
KEEP=0

# Possible test styles are here.
# ------------------------------
#	no_url 
# no_url+headers 
# url
#	url+headers 
#	get_only 
#	get+url 
#	get+headers 
#	get+headers+url 
#	post_only 
#	post+url 
#	post+headers 
#	post+headers+url 
#	post+headers+get 
#	post+headers+get+url
# 
# Uncomment the ones you'd like to use below.
TESTCASES=(get_only get+url get+headers+url)
#TESTCASES=(post+headers+get+url)

#TESTCASES=(no_url)
#TESTCASES=(no_url no_url+headers)
#TESTCASES=(no_url no_url+headers url)
#TESTCASES=(no_url no_url+headers url url+headers)
#TESTCASES=(no_url no_url+headers url url+headers get_only)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only post+url)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only post+url post+headers)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only post+url post+headers post+headers+url)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only post+url post+headers post+headers+url post+headers+get)
#TESTCASES=(no_url no_url+headers url url+headers get_only get+url get+headers get+headers+url post_only post+url post+headers post+headers+url post+headers+get post+headers+get+url)


# warn me 
warn() {
	printf "$THIS: $1\n" > /dev/stderr
}


# error out
err() {
	printf "$THIS: $1\n" > /dev/stderr
	exit ${2:-1}
}

# keep this little sanity check in here
test -z "$TESTCASES" && {
	err "No test cases specified (uncomment one of the TESTCASES options in $0)."
}


# show usage
usage() {
cat <<EOF
-f, --file <arg>          Load urls from this file.
    --sleep               Sleep for <arg> seconds between requests.
-u, --parallel            Run in parallel.
    --randomize-clients   Randomize clients used to invoke things.
-c, --client <arg>        Use only this client to test.
-s, --server <arg>        Use this specific server.
-p, --port <arg>          Use this port.
-w, --wait                Wait for input before running a test command.
    --dry-run             Do not actually do anything, but show what would be done.
    --keep                Keep the result set
-i, --init-tests          Initialize the test files.
-r, --records <arg>       Generate <arg> records when generating tests.
-v, --verbose             Be verbose and spit out commands before we execute them.
-h, --help                Show help
EOF
exit $1
}


# generate random string
randstr() {
	local ALPHABET="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	local LENGTH=${1:-32}
	local RANDSTR=
	for l in `seq 1 $LENGTH`
	do
		RANDSTR+=${ALPHABET:$(( $RANDOM % ${#ALPHABET} )):1}
	done
	echo $RANDSTR
}


# Process options
while [ $# -gt 0 ]
do
	case "$1" in
		# Load sites from a file
		-f|--file)
			shift
			URLFILE="$1"
			test -f "$URLFILE" || err "No file specified."
		;;

		# Run parallel?
		-u|--parallel)
			THREAD=1
		;;

		# Run parallel?
		--randomize-clients)
			RANDOMIZE_CLIENTS=1
		;;

		# Choose a specificc client 
		-c|--client)
			shift
			CLIENT=$1
		;;

		# Choose a different server
		-s|--server)
			shift
			SERVER=$1
		;;

		# Choose a specific port 
		-p|--port)
			shift
			WWWPORT=$1
		;;


		# Place tests in this directory (/tmp/hypno-tests is default)
		-d|--dir)
			shift
			OUTPUT_DIR=$1
		;;

		-w|--wait)
			WAIT_FOR_INPUT=1
			VERBOSE=1
		;;
		--sleep)
			SLEEP=1
			VERBOSE=1
			shift
			SLEEP_INTERVAL=$1
		;;
		--dry-run)
			DRYRUN=1
			VERBOSE=1
		;;
		-v|--verbose)
			VERBOSE=1
		;;
		-h|--help)
			usage 0	
		;;
		-i|--init-tests)
			INITTEST=1
		;;
		--keep)
			KEEP=1
		;;
		-r|--records)
			shift
			RECORDS=$1
		;;
	esac
	shift
done



# Initialize the test suite.
inittest() {

	# Define all the constants
	TESTSQL=tests/test.sql
	TESTDB=tests/test.db
	TC_WORDS=tests/words
	TC_CWORDS=tests/words_no_apostrophe
	TC_BLOCKS=tests/wordblocks
	SED_URLENCODER=tests/urlencode.sed

	# Generate a table from scratch
	echo "
		CREATE TABLE t ( 
			uuid INTEGER PRIMARY KEY AUTOINCREMENT, 
			id TEXT, 
			url TEXT, 
			curl_headers TEXT, 
			wget_headers TEXT, 
			chrome_headers TEXT, 
			get TEXT, 
			curl_body TEXT, 
			wget_body TEXT, 
			chrome_body TEXT,
			expected_response TEXT 
		);
	" > $TESTSQL

	# Generate records for $RECORDS amount 
	for i in `seq 0 $RECORDS`
	do 
		# 
		test $VERBOSE -eq 1 && echo Generating record $i > /dev/stderr

		# 
		WL=`wc -l $TC_WORDS | awk '{ print $1 }'`; 
		BL=`wc -l $TC_BLOCKS | awk '{ print $1 }'`; 
		WNOL=`wc -l $TC_CWORDS | awk '{ print $1 }'`;  

		# Generate random header data 
		HEADER=
		CURL_HEADER=
		WGET_HEADER=
		CHROME_HEADER=
		HEADER_ARR=
		for n in `seq 0 $(( $RANDOM % 20 ))`
		do 
			HEADERBLOCK="X-header-xxxx-`sed -n "$(( $RANDOM % $WNOL ))p" $TC_CWORDS`: `sed -n "$(( $RANDOM % $WNOL ))p" $TC_CWORDS`"; 
			HEADER+="$HEADERBLOCK"; 
			CURL_HEADER+=" -H '$HEADERBLOCK'"; 
			WGET_HEADER+=" --header '$HEADERBLOCK'"; 
			CHROME_HEADER+="$HEADERBLOCK"; 
			HEADER_ARR[ $n ]="$HEADERBLOCK<br>"; 
		done

		# Generate random query string data.
		GET= 
		GET_ARR=
		for n in `seq 0 $(( $RANDOM % 20 ))`
		do 
			GETBLOCK="&`sed -n "$(( $RANDOM % $WNOL ))p" $TC_CWORDS`=`sed -n "$(( $RANDOM % $BL ))p" $TC_BLOCKS | sed -f $SED_URLENCODER`"; 
			GET+="$GETBLOCK" 
			GET_ARR[ $n ]="$GETBLOCK<br>"; 
		done

		# Generate random POST body data.
		BODY= 
		CURL_BODY= 
		WGET_BODY= 
		CHROME_BODY= 
		BODY_ARR= 
		for n in `seq 0 $(( $RANDOM % 13 ))`
		do 
			BODYBLOCK="`sed -n "$(( $RANDOM % $BL ))p" $TC_BLOCKS`"; 
			INNERKEY=`randstr $(( $RANDOM % 37 ))`
			BODY+="$BODYBLOCK"; 
			CURL_BODY+=" --data-urlencode '$INNERKEY=$BODYBLOCK'"; 
			WGET_BODY+="&$INNERKEY=$BODYBLOCK"; 
			CHROME_BODY+="$INNERKEY=$BODYBLOCK"; 
			BODY_ARR[ $n ]="$INNERKEY=$BODYBLOCK<br>\n"; \
		done; 

		# Generate random URI data 
		URLBODY= 
		URLBODY_ARR= 
		for n in `seq 0 $(( $RANDOM % 13 ))`
		do 
			URLBLOCK="/`sed -n "$(( $RANDOM % $WNOL ))p" $TC_CWORDS`"; 
			URLBODY+="$URLBLOCK"; 
			URLBODY_ARR[ $n ]="$URLBLOCK"; 
		done

		# Response generation
		RESPONSE_FILE=/tmp/shimmy
		test -f $RESPONSE_FILE && rm $RESPONSE_FILE
		printf "<h2>URL</h2>\n$URLBODY<br>\n" >> $RESPONSE_FILE;

		printf "\n<h2>Headers</h2>" >> $RESPONSE_FILE;
		for n in ${HEADER_ARR[@]}; do 
			printf "$n" | sed 's/:/ => /; s/X-/\nX-/g'
		done >> $RESPONSE_FILE

		printf "\n\n<h2>GET</h2>" >> $RESPONSE_FILE;
		for n in ${GET_ARR[@]}; do 
			printf "$n " | sed 's/=/ => /; s/&/\n/g'
		done >> $RESPONSE_FILE

		printf "\n\n<h2>POST</h2>\n" >> $RESPONSE_FILE;
		for n in ${BODY_ARR[@]}; do 
			printf "$n " | sed 's/=/ => /'
		done >> $RESPONSE_FILE

		# Generate SQL statement 
		echo "INSERT INTO t VALUES( 
			 NULL, 
			\"`randstr 9`\", 
			\"$URLBODY\", 
			\"`printf -- "$CURL_HEADER"`\",
			\"`printf -- "$WGET_HEADER"`\",
			\"`printf -- "$CHROME_HEADER"`\",
			\"`printf -- "$GET" | sed 's/^\&/?/'`\",
			\"`printf -- "$CURL_BODY"`\",
			\"`printf -- "$WGET_BODY"`\",
			\"`printf -- "$CHROME_BODY"`\",
			\"`cat $RESPONSE_FILE | sed 's/^ //'`\"
		 );" >> $TESTSQL
	done; 


	# Load all the data
	test -f $TESTDB && rm $TESTDB
	sqlite3 $TESTDB < $TESTSQL || err "Failed to generate test suite..." 1
}



# Runs a chosen client, placing its output in a variety of files.
xclient() {
	# This is here so that servers can be randomized in the future.
	THE_SERVER=$SERVER
	#SITE=`echo $1 | awk '{ print $1 }'`
	SITE=$1
	TESTTYPE=$2
	CLI=$3

	# Constants: Templates for commands, etc
	CURL_CMD="curl -D /dev/stderr --silent --no-styled-output --connect-timeout 1 "
	WGET_CMD="wget --timeout=1 --tries=2 -S -O- "
	CHROME_CMD=
	sqlite3="sqlite3 tests/test.db"

	# Format strings
	sql_fmt="select ${CLI}_headers,${CLI}_body,url,get from t where uuid = 1;"
	awk_fmt=

	# Define places for content
	FILEBASE=$OUTPUT_DIR/`randstr 32`
	PAGE_ADDR=${FILEBASE}-addr
	PAGE_CLIENT=${FILEBASE}-client
	PAGE_HEADERS=${FILEBASE}-headers
	PAGE_CONTENT=${FILEBASE}-content
	PAGE_STATUS=${FILEBASE}-status
	PAGE_LENGTH=${FILEBASE}-length
	PAGE_DATE=${FILEBASE}-date
	PAGE_METHOD=${FILEBASE}-method
	PAGE_EXPECTED=${FILEBASE}-expected
	PAGE_URL=${FILEBASE}-url

	# Write the client, date, address and ID to individual files
	date --rfc-3339=ns > $PAGE_DATE
	basename $FILEBASE > $FILEBASE
	echo $SITE > $PAGE_ADDR
	echo $CLI > $PAGE_CLIENT

	# ...
	printf '' > $PAGE_URL

	# Decide how to format the test
	case $TESTTYPE in
		head)                 awk_fmt='{ printf "%s \"%s\"\n", CLIENT, SITE }'  
		;;
		no_url)               awk_fmt='{ printf "%s \"%s\"\n", CLIENT, SITE }'  
		;;
		no_url+headers)       awk_fmt='{ printf "%s %s \"%s\"\n", CLIENT, $1, SITE }'  
		;;
		url)                  awk_fmt='{ printf "%s \"%s%s\"\n", CLIENT, SITE, $3 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		;;
		url+headers)          awk_fmt='{ printf "%s %s \"%s%s\"\n", CLIENT, $1, SITE, $3 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		;;
		get_only)             awk_fmt='{ printf "%s \"%s%s\"\n", CLIENT, SITE, $4 }'  
		;;
		get+url)              awk_fmt='{ printf "%s \"%s%s%s\"\n", CLIENT, SITE, $3, $4 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $4 }' >> $PAGE_URL
		;;
		get+headers)          awk_fmt='{ printf "%s %s \"%s%s\"\n", CLIENT, $1, SITE, $4 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $4 }' > $PAGE_URL
		;;
		get+headers+url)      awk_fmt='{ printf "%s %s \"%s%s%s\"\n", CLIENT, $1, SITE, $3, $4 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $4 }' >> $PAGE_URL
		;;
		post_only)            awk_fmt='{ printf "%s %s \"%s\"\n", CLIENT, $2, SITE }'  
		;;
		post+url)             awk_fmt='{ printf "%s %s \"%s%s\"\n", CLIENT, $2, SITE, $3 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		;;
		post+headers)         awk_fmt='{ printf "%s %s %s \"%s\"\n", CLIENT, $1, $2, SITE }'  
		;;
		post+headers+url)     awk_fmt='{ printf "%s %s %s \"%s%s\"\n", CLIENT, $1, $2, SITE, $3 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		;;
		post+headers+get)     awk_fmt='{ printf "%s %s %s \"%s%s\"\n", CLIENT, $1, $2, SITE, $4 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $4 }' > $PAGE_URL
		;;
		post+headers+get+url) awk_fmt='{ printf "%s %s %s \"%s%s%s\"\n", CLIENT, $1, $2, SITE, $3, $4 }'  
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $3 }' > $PAGE_URL
		$sqlite3 "$sql_fmt" | awk -F'|' '{ print $4 }' >> $PAGE_URL
		;;
	esac

	# Guess the intended method by the command line arguments
	# test ${TESTTYPE:0:1} == 'p' && echo POST || echo GET > $PAGE_METHOD
	echo $TESTTYPE > $PAGE_METHOD

	# Choose the command to execute here.
	case $CLI in
		curl)
			CLIENT_CMD=`$sqlite3 "$sql_fmt" | \
				awk -vSITE="$SITE" -vCLIENT="$CURL_CMD" -F'|' "$awk_fmt"`
		;;
		wget)
			CLIENT_CMD=`$sqlite3 "$sql_fmt" | \
				awk -vSITE="$SITE" -vCLIENT="$WGET_CMD" -F'|' "$awk_fmt"`
		;;
		*)
			echo Invalid client "$CLI".  Stopping...
			exit
		;;
	esac


	# Test the differences
	test $VERBOSE -eq 1 && echo $CLIENT_CMD > /dev/stderr
	test $DRYRUN -eq 1 && return 
	test $SLEEP -eq 1 && sleep $SLEEP_INTERVAL
	test $WAIT_FOR_INPUT -eq 1 && echo "Awaiting user input..." >/dev/stderr
	test $WAIT_FOR_INPUT -eq 1 && read

	# Execute command
	sh -c "$CLIENT_CMD 2>$PAGE_HEADERS >$PAGE_CONTENT" || {
		printf "Client call failed...\n" >/dev/stderr
		if [ 0 -eq 1 ]
		then
			return
		else
			test $KEEP -eq 0 && rm -rf /tmp/hypno-tests	
			exit
		fi	
	}
	grep "HTTP/[01].[01]" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_STATUS
	grep "Content-Length:" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_LENGTH

	# Get the expected payload, and compare it to content.
	# ...
	# 1. memcmp( x, y, xlen );
	# 2. {md5,sha1,sha256}sum of x & y
	# 3. xxd x > $FILE_1 xxd y > $FILE_2 && diff $FILE_1 $FILE_2
	# 4. SELECT * FROM payload WHERE expected = %s 
}



# Generate a test suite...
if [ $INITTEST -eq 1 ]
then
	test $VERBOSE -eq 1 && echo "Generating test suite..." > /dev/stderr
	inittest
	test $VERBOSE -eq 1 && echo "DONE!" > /dev/stderr
	exit 0
fi



# Quick checks for an output directory... 
test ! -z "$OUTPUT_DIR" || err "No output directory specified."
test -d $OUTPUT_DIR || mkdir -p $OUTPUT_DIR || err "Could not create output directory."


# Debug and dump
if [ 1 -eq 1 ]
then
	cat <<EOF
Running tests against server '$SERVER' with the following parameters:
EOF
fi


# Choose default site and set default client.
line="$WWWADDR:$WWWPORT"
CLIENT=${CLIENT:-curl}

# TODO: Use SQlite to get a count of values (or just specify on the command line)
COUNT=0
for i in `seq 0 $COUNT`
do
	# This should be run somewhere.  It just depends on the level of testing you want.
	#[ $RANDOMIZE_CLIENTS -eq 1 ] && CLIENT=${CLIENTS[ $(( $RANDOM % ${#CLIENTS[@]} )) ]}

	# Loop through test cases.
	for m in ${TESTCASES[@]}
 	do
		#echo xclient "$line" "$m" "$CLIENT" && continue
		if [ $THREAD -eq 0 ]
		then
			xclient "$line" "$m" "$CLIENT"
		else
			xclient "$line" "$m" "$CLIENT" &
			PID=$!
			echo $PID
		fi
	done
done
exit


# If it's a dry run, nothing happened, so stop.
[ $DRYRUN -eq 1 ] && exit 0


# Let all background processes come to an end.
[ $THREAD -eq 1 ] && wait


# Reassemble payload requests
printf "
	<table>
		<thead>
			<th>ID</th>
			<th>Length</th>
			<th>Status</th>
			<th>Headers</th>
			<th>Content</th>
			<th>URL Requested</th>
			<th>Method</th>
			<th>Request Date</th>
			<!-- <th>Successful?</th> -->
		</thead>
"

find $OUTPUT_DIR -type f ! -name "*-*" | \
	xargs -IFF sh -c '
	printf "<tr>\n"
	for n in id length status headers content addr method date; do
		if [[ $n == "id" ]]; then
			printf -- "<td>`cat FF | head -c 6`</td>\n"
		elif [[ $n == "headers" ]]; then
			printf -- "<td><pre>`cat FF-$n`</pre></td>\n"
		elif [[ $n == "addr" ]]; then
			printf -- "<td>`cat FF-$n``cat FF-url`</td>\n"
		elif [[ $n == "content" ]]; then
			printf -- "<td><pre>"
			printf -- "`cat FF-$n | sed \"s/</\&lt;/g\" | sed \"s/>/\&gt;/g\"`"
			printf -- "</pre></td>\n"
		else
			printf -- "<td>`cat FF-$n`</td>\n"
		fi
	done
	printf "</tr>\n"
	'

printf "
	</table>
<style>
th { text-align: left; }
tr:nth-child(even) { background-color: #ddd; }
</style>
	"

# Get rid of the output directory
test $KEEP -eq 0 && rm -rf $OUTPUT_DIR

# Return with success
exit 0



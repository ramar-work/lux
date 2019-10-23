#!/bin/bash -
#
# The most solid way to run this is against an echo server.
#
# Test parameters:
# - Do the 4 supported HTTP methods work for each endpoint you're running against?
#		We don't support CONNECT, TRACE, or OPTIONS (maybe OPTIONS in the future)
# - Does the expected response match the expected response.
# - Was the request successful (or was there some kind of crash?)
# - Was invalid input processed?
#  
THIS=hypnotest
CONST_DB=126
CONST_FILE=127
OUTPUT_TYPE=$CONST_FILE
CLIENTS="wget|curl|chromium|opera"
CLIENT=
SERVER=bin/hypno
WWWADDR=http://localhost
WWWPORT=2000

THREAD=0
OUTPUT_DIR=/tmp/hypno-tests

RANDOMIZE_CLIENTS=0
URLFILE=

# warn me 
warn() {
	printf "$THIS: $1\n" > /dev/stderr
}

# error out
err() {
	printf "$THIS: $1\n" > /dev/stderr
	exit ${2:-1}
}

# show usage
usage() {
cat <<EOF
-f, --file <arg>           Load urls from this file.
-u, --parallel             Run in parallel.
    --randomize-clients    Randomize clients used to invoke things.
-c, --client <arg>         Use only this client to test.
-s, --server <arg>         Use this specific server.
-p, --port <arg>           Use this port.
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

newrand() {
	echo $(( $RANDOM % 127 ))
}



# Die at no options
#[ $# -eq 0 ] && usage 0


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
	esac
	shift

done


# ....
test ! -z "$OUTPUT_DIR" || {
	err "No output directory specified."
}

test -d $OUTPUT_DIR || mkdir -p $OUTPUT_DIR || {
	err "Could not create output directory."
}



# Debug and dump
if [ 1 -eq 1 ]
then
	cat <<EOF
Running tests against server '$SERVER' with the following parameters:
EOF
fi



# For the sake of keeping things clear, this function goes here
xclient() {
	# This is here so that servers can be randomized in the future.
	THE_SERVER=$SERVER
	THE_ADDR=`echo $1 | awk '{ print $1 }'`
	CLI=curl

	# Generate headers, GET, POST and PUT body
	CURL_CMD="curl --silent -i --no-styled-output --connect-timeout 1 "
	WGET_CMD="wget --timeout=1 --tries=2 -S -O- "
	CHROME_CMD=

	# This should be a for loop that tries a variety of things...
	sql_fmt="select ${CLI}_headers,${CLI}_body,url,get from t where uuid = 1;"
	site="http://localhost:2000"
	sqlite3="sqlite3 tests/test.db"

	# Choose the different formats for testing here
	# 
  # Different situations are listed below:	
	# also need to add different methods
	#	no_url 
	# no_url+headers 
	# url
	#		url+headers \
	#		get_only \
	#		get+url \
	#		get+headers \
	#		get+headers+url \
	#		post_only \
	#		post+url \
	#		post+headers \
	#		post+headers+url \
	#		post+headers+get \
	#		post+headers+get+url
	#
	for m in \
		no_url no_url+headers url
		
		
	do
		# Definitions all day.
		awk_fmt=
		method_test=

		# Define places for content
		FILEBASE=$OUTPUT_DIR/`randstr 32`
		PAGE_ADDR=${FILEBASE}-addr
		PAGE_CLIENT=${FILEBASE}-client
		PAGE_HEADERS=${FILEBASE}-headers
		PAGE_CONTENT=${FILEBASE}-content
		PAGE_STATUS=${FILEBASE}-status
		PAGE_LENGTH=${FILEBASE}-length
		PAGE_DATE=${FILEBASE}-date

		# Write the client, date, address and ID to individual files
		if [ 1 -eq 1 ]
		then
			date --rfc-3339=ns > $PAGE_DATE
			basename $FILEBASE > $FILEBASE
			echo $THE_ADDR > $PAGE_ADDR
			echo $CLI > $PAGE_CLIENT
		fi

		# Decide how to format the test
		case $m in
			no_url)               awk_fmt='{ printf "%s %s\n", CLIENT, SITE }'  ;;
			no_url+headers)       awk_fmt='{ printf "%s %s %s\n", CLIENT, $1, SITE }'  ;;
			url)                  awk_fmt='{ printf "%s %s%s\n", CLIENT, SITE, $3 }'  ;;
			url+headers)          awk_fmt='{ printf "%s %s %s%s\n", CLIENT, $1, SITE, $3 }'  ;;
			get_only)             awk_fmt='{ printf "%s %s%s\n", CLIENT, SITE, $4 }'  ;;
			get+url)              awk_fmt='{ printf "%s %s%s%s\n", CLIENT, SITE, $3, $4 }'  ;;
			get+headers)          awk_fmt='{ printf "%s %s %s%s\n", CLIENT, $1, SITE, $4 }'  ;;
			get+headers+url)      awk_fmt='{ printf "%s %s %s%s%s\n", CLIENT, $1, SITE, $3, $4 }'  ;;
			post_only)            awk_fmt='{ printf "%s %s %s\n", CLIENT, $2, SITE }'  ;;
			post+url)             awk_fmt='{ printf "%s %s %s%s\n", CLIENT, $2, SITE, $3 }'  ;;
			post+headers)         awk_fmt='{ printf "%s %s %s %s\n", CLIENT, $1, $2, SITE }'  ;;
			post+headers+url)     awk_fmt='{ printf "%s %s %s %s%s\n", CLIENT, $1, $2, SITE, $3 }'  ;;
			post+headers+get)     awk_fmt='{ printf "%s %s %s %s%s\n", CLIENT, $1, $2, SITE, $4 }'  ;;
			post+headers+get+url) awk_fmt='{ printf "%s %s %s %s%s%s\n", CLIENT, $1, $2, SITE, $3, $4 }'  ;;
		esac

		# Write the current method to file too
		method_test=$m

		# This is what I'm working with now.
		case $CLI in
			curl)
				CLIENT_CMD=`$sqlite3 "$sql_fmt" | \
					awk -vSITE="$site" -vCLIENT="$CURL_CMD" -F'|' "$awk_fmt"`
#echo $CLIENT_CMD; continue;

				BF=/tmp/`randstr 64`
				$CLIENT_CMD > $BF || {
					echo "Failed to make request... ?"
					return
				}

				# Manually split what was returned by curl.
				CHOPLINE=`grep --line-number $'\x0D' $BF | \
					sed -n '/[0-9]:\r$/ p' | sed 's/:.*//'`	
				LINECOUNT=$( wc -l $BF | awk '{ print $1 }' )

				# Then start playing with files
				sed -n "1,$(( $CHOPLINE - 1 ))p" $BF > $PAGE_HEADERS
				sed -n "${CHOPLINE},$(( $LINECOUNT + 1 ))p" $BF > $PAGE_CONTENT
				grep "HTTP/[01].[01]" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_STATUS
				grep "Content-Length:" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_LENGTH

				# Get rid of this temp file.
				rm -f $BF
			;;
			wget)
				CLIENT_CMD=`$sqlite3 "$sql_fmt" | awk -vSITE="$site" -vCLIENT="$WGET_CMD" -F'|' "$awk_fmt"`
echo $CLIENT_CMD; continue;

				CLIENT_CMD=$( 
					sqlite3 tests/test.db "select ${CLI}_headers,${CLI}_body,url,get from t where uuid = 1;" | \
						awk \
							-vSITE="http://localhost:2000" \
							-vCLIENT="$WGET_CMD" \
							-F '|' '{
								printf "%s %s %s %s%s%s\n", CLIENT, $1, $2, SITE, $3, $4
						}' 
					)

				# Execute and write everything where it needs to be.
				echo $CLIENT_CMD; return;
				$CLIENT_CMD >$PAGE_CONTENT 2>$PAGE_HEADERS
				grep "HTTP/[01].[01]" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_STATUS
				grep "Content-Length:" $PAGE_HEADERS | awk '{ print $2 }' > $PAGE_LENGTH
			;;
			chromium)
			;;
			chrome)
			;;
			opera)
			;;
			edge)
			;;
		esac
	done

}


# Run the test
line="$WWWADDR:$WWWPORT"
# TODO: Use SQlite to get a count of values (or just specify on the command line)
COUNT=1
for i in `seq 0 $COUNT`
do
	if [ $THREAD -eq 0 ]
	then
		xclient "$line"
	else
		xclient "$line" &
		PID=$!
		echo $PID
	fi
done


#Show things
#echo 'requests may or may not have finished...'

# Let all background processes come to an end.
[ $THREAD -eq 1 ] && wait


# Using SQLite would be better for this.  You can pull out all the failures
# on the fly w/o xargs magic...



# Reassembly should take place somewhere
# Though you have some choices on how this can look.
# sqlite3 was my first choice...
# html is the current one...


# TODO: C structures make this a LOT simpler. 
# 200 or 500? (when expected)
# Content should match what was sent
printf "
	<table>
		<thead>
			<th>ID</th>
			<th>Length</th>
			<th>Status</th>
			<th>Headers</th>
			<th>Addr</th>
			<th>Request Made</th>
			<!-- <th>Method</th> -->
			<!-- <th>Successful?</th> -->
			<th>Content</th>
		</thead>
"

find $OUTPUT_DIR -type f ! -name "*-*" | \
	xargs -IFF sh -c '
	printf "<tr>\n"
	printf "<td>`cat FF`</td>\n"
	for n in length status content headers addr date; do
		if [[ $n == "headers" ]]
		then
			printf "<td><pre>`cat FF-$n`</pre></td>\n"
		elif [[ $n == "content" ]]
		then
			printf "<td><pre>"
			printf "`cat FF-$n | sed "s/</\&lt;/g" | sed "s/>/\&gt;/g"`"
			printf "</pre></td>\n"
		else
			printf "<td>`cat FF-$n`</td>\n"
		fi
		rm FF-$n
	done
	rm FF
	printf "</tr>\n"
	'

printf "
	</table>
<style>
th { text-align: left; }
</style>
	"

# Get rid of the output directory
rm -rf $OUTPUT_DIR
exit






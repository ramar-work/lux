#!/bin/bash -
#Run each command from a file.
#You could save an individual file (since there is no limit to number of inodes 
#on a system (well there is, but it's ridiculously high).  With these short
#requests, it's unlikely that you'll exceed 1K (or even 10K connections)
#At the end of the process, you could do something

THIS=hypnotest
CONST_DB=126
CONST_FILE=127
OUTPUT_TYPE=$CONST_FILE
CLIENTS="wget|curl|chromium|opera"
CLIENT=
SERVER=bin/hypno
PORT=2000

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



# Die at no options
[ $# -eq 0 ] && usage 0


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
			PORT=$1
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


test ! -z "$URLFILE" || {
	err "No URL file specified.  No tests to run."
}


test -f "$URLFILE" || {
	err "The given URL file does not seem to exist."
}


test ! -z "$SERVER" || {
	err "No server specified."
}	


# Debug and dump
if [ 1 -eq 1 ]
then
	cat <<EOF
Running tests against server '$SERVER' with the following parameters:
EOF
fi


# Update the port 
test -f /tmp/urlcmds && rm -f /tmp/urlcmds
sed "s;@@PORT@@;$PORT;" $URLFILE > /tmp/urlcmds 


# For the sake of keeping things clear, this function goes here
xclient() {
	# This is here so that servers can be randomized in the future.
	THE_SERVER=$SERVER
	echo $1
	case 0 in
		# Each client invocation should be different, but 
		# obviously must result in the same information.
		curl)
		;;
		chromium)
		;;
		chrome)
		;;
		wget)
			wget \
				--timeout=1 \
				--tries=2 \
				-S -O/dev/null \
				http://`basename DD` 2>&1
		;;
	esac
}


# Read each line and do something
while read line
do
#FILEBASE=$OUTPUT_DIR/`randstr 16`
	# If the first character is a comment, continue
	[ ${line:0:1} == '#' ] && continue

	# Choose an output file
	FILEBASE=/dev/stdout

	# Run the test
	if [ $THREAD -eq 0 ]
	then
		xclient "$line" > $FILEBASE
	else
		xclient "$line" > $FILEBASE &
 		PID=$!
		echo $PID
	fi

done < /tmp/urlcmds



if [ $THREAD -eq 1 ]
then
	# Let all background processes come to an end.
	wait
fi




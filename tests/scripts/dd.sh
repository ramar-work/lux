#!/bin/bash -
# ========================================================
# dd.sh 
# -----
# Generates large tables in Lua for testing against
# my own functions.  Specifically:
# 
# - lua_stackdump( ... )
# - lua_to_table( ... )
# - lua_agg( ... )
#
# ========================================================
COUNT=100
DEPTH=1
MAX_DEPTH=15
WF=/usr/share/dict/words
TABS="         "

function table_value 
{
	printf "${TABS:0:${DEPTH}}"

	#String, number or table
	case $(( $RANDOM % 3 )) in
		0)
			sed -n ${RANDOM}p $WF | sed 's/$/,/'
			;;
		1)
			printf "%d," $RANDOM	
			;;
		2)
			#Generate either a string or number as index
			[ $(( $RANDOM % 2 )) -eq 0 ] && \
				sed -n ${RANDOM}p $WF | sed 's/$/ = {/' || printf "[%d] = {" $RANDOM	

			#If depth is greater than 9, back up and close the current table.
			if [ $DEPTH -gt $MAX_DEPTH ]
			then
				printf "${TABS:0:${DEPTH}} },"
			else
				let "DEPTH += 1"
				# Handle recursion control from here
				INNER=$(( $RANDOM % 9 ))
				while [ $INNER -gt 0 ]
				do
					table_value
					printf "${TABS:0:${DEPTH}}"
					let "INNER -= 1"
				done
				let "DEPTH -= 1"
				printf "${TABS:0:${DEPTH}} },"
			fi
			;;
	esac
	printf "\n"
} 

printf "return {\n"

while [ $COUNT -gt 0 ]
do
	table_value
	let "COUNT -= 1"
done
	
printf "}\n"	



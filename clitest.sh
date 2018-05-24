#!/bin/bash -

# Create the database because it's the first run
valgrind ./hypnob --list

# Should show a list of tables.
if [ `sqlite3 ~/.hypno/hypno.db '.tables' | wc -l | head -n 1` -lt 1 ]
then
	printf "Tables didn't generate.\n" > /dev/stderr
fi

# Delete everything and start again
rm -rf ~/.hypno

#!/bin/bash
for i in `find -L $1`
do
	if test -L "$i" && ! test -e "$i" && [ $(($(date '+%s') - $(stat -c '%Y' "$i"))) -ge $((60*60*24*7)) ]
	then
		echo "$i"
	fi
done

#!/bin/bash
for i in `find -L $1`
do
	if test -L "$i" -a ! -e "$i" -a $(($(date '+%s') - $(stat -c '%Y' "$i"))) -ge $((60*60*24*7))
	then
		echo "$i"
	fi
done

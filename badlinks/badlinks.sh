for i in `find`
do
	test -L "$i" && ! test -e "$i" && test "$((`date '+%s'` - `stat -c '%Y' "$i"` > 60*60*24*7))" && echo "$i"
done

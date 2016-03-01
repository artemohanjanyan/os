#!/bin/bash
# ^ можно не только баш, можно даже питон

A=5
echo $A

for i in `ls`
do
	echo $i
done

for ((i=0;i<20;++i))
do
	echo $i
done

A=123
B=123

if [ "$A" = "$B" ]
then
	echo equal
fi

echo

# globbing

for ((i=0;i<10;++i))
do
	case $i in
		1)
			echo 2
			;;
		5)
			echo 321
			;;
		*)
			echo $i
			;;
	esac
done

# https://github.com/uuner/sedtris.git

#. - любой символ
#[abc] [a-z] [a-zA-Z]
#R*
#R+
#R?
#^ $
#A|B
#(R) - найти и запомнить
#\0 \1 \2 - обратиться к запомненному
#	например (a+)b\1
# xargs

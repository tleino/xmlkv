#!/bin/sh

for a in lynx curl nroff kvxml ; do
	which $a 2>/dev/null 1>/dev/null
	if [ $? -ne 0 ] ; then
		echo "$0: install $a or add to PATH"
		exit 1
	fi
done

if [ ! -e slashdot.rss ] ; then
	curl \
		http://rss.slashdot.org/slashdot/slashdotMain?format=xml \
		>slashdot.rss
fi

if [ $1 ] ; then
	while [ $1 ] ; do
		cat slashdot.rss\
			|kvxml -sq item/title$\
			|awk '{ print toupper($0) }'\
			|sed -n "$1p"\
			|fmt
		cat slashdot.rss\
			|kvxml -sq item/description$\
			|sed "s/^'//"\
			|sed -n "$1p"\
			|lynx -dump -stdin -nolist\
			|sed '/^$/q'
		shift
	done
else
	cat slashdot.rss\
		|kvxml -sq item/title$\
		|sed "s/^'//"\
		|awk '{ print ".IP",NR; print }'\
		|nroff -ms\
		|uniq
fi

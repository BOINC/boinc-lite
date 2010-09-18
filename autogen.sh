#/bin/sh
autoscan 2>&1 | grep -v cellos
autoheader
aclocal
libtoolize -f
autoconf
automake -a -Wall

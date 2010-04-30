#!/bin/sh
libtoolize --force
aclocal
autoheader
autoconf
automake --add-missing

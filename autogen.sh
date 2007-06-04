#!/bin/sh
#
aclocal $ACLOCAL_FLAGS
automake --foreign
autoconf

#./configure $*
echo "Now you are ready to run ./configure"

#! /bin/sh
set -x
aclocal
autoheader
automake --copy --add-missing
autoconf

#!/bin/bash

VERSION=
REVISION=

if [ -z "$VERSION" ]; then
	VERSION='0.5.0'
	if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
		REVISION=$(git describe --tags --long)
		VERSION=$(git describe --tags|sed 's,[-_],.,g;s,\.g.*$,,')
		(
		   echo -e "# created with: git log --stat=76 | fmt -sct -w80\n"
		   git log --stat=76 | fmt -sct -w80
		)>ChangeLog
	fi
fi

# if you don't have -i get a real version of sed!!!
sed -r -e -i \
    "s:AC_INIT\(.*:AC_INIT([waimea],[$VERSION],[https://github.com/bbidulock/waimea/issues]):
     s:AC_REVISION\(.*:AC_REVISION([$REVISION]):" \
    configure.ac

autoreconf -fiv

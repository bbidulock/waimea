SUBDIRS = actions styles backgrounds
CLEANFILES = waimearc.example
pkgdata_DATA = menu

all-local: waimearc.example

waimearc.example: waimearc.example.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," waimearc.example.in > waimearc.example

SUBDIRS = actions styles backgrounds
CLEANFILES = config
pkgdata_DATA = menu config

all-local: config

config: config.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," config.in > config

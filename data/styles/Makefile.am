CLEANFILES = Default
stylesdir = $(pkgdatadir)/styles
styles_DATA = Default

all-local: Default

Default: Default.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," Default.in > Default

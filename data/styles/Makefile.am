CLEANFILES = Default.style
stylesdir = $(pkgdatadir)/styles
styles_DATA = Default.style

all-local: Default.style

Default.style: Default.style.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," Default.style.in > Default.style

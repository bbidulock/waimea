CLEANFILES = stylesdir.pl
scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = stylesdir.pl

all-local: stylesdir.pl

stylesdir.pl: stylesdir.pl.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," stylesdir.pl.in > stylesdir.pl

CLEANFILES = stylesdir.pl
scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = stylesdir.pl filelist.pl

all-local: stylesdir.pl filelist.pl

stylesdir.pl: stylesdir.pl.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," stylesdir.pl.in > stylesdir.pl


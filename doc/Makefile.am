CLEANFILES = waimea.1
man_MANS = waimea.1

all-local: waimea.1

waimea.1: waimea.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," waimea.1.in > waimea.1

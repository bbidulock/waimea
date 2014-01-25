
nobase_dist_pkgdata_DATA = \
	backgrounds/waimea.png \
	actions/action.sloppy-focus \
	actions/action.click-to-focus \
	actions/action.non-opaque \
	menu

nobase_dist_pkgdata_SCRIPTS = \
	scripts/filelist.pl \
	scripts/procinfo.pl

nobase_nodist_pkgdata_DATA = \
	styles/Default.style \
	config

nobase_nodist_pkgdata_SCRIPTS = \
	scripts/stylesdir.pl

nobase_dist_data_DATA = \
	xsessions/waimea.desktop

install-data-hook:
	rm -f "$(DESTDIR)$(pkgdatadir)/actions/action"
	$(LN_S) action.sloppy-focus "$(DESTDIR)$(pkgdatadir)/actions/action"

uninstall-local:
	rm -f "$(DESTDIR)$(pkgdatadir)/actions/action"



nobase_pkgdata_DATA = \
	backgrounds/waimea.png \
	scripts/filelist.pl \
	scripts/procinfo.pl \
	actions/action.sloppy-focus \
	actions/action.click-to-focus \
	actions/action.non-opaque \
	menu

nodist_nobase_pkgdata_DATA = \
	styles/Default.style \
	scripts/stylesdir.pl \
	config

nobase_data_DATA = \
	xsessions/waimea.desktop

install-data-hook:
	rm -f "$(DESTDIR)$(pkgdatadir)/actions/action"
	$(LN_S) action.sloppy-focus "$(DESTDIR)$(pkgdatadir)/actions/action"

uninstall-local:
	rm -f "$(DESTDIR)$(pkgdatadir)/actions/action"


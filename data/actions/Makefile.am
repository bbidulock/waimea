actionsdir = $(pkgdatadir)/actions
actions_DATA = action.sloppy-focus \
               action.click-to-focus \
               action.non-opaque

install-data-local:
	cd $(DESTDIR)$(pkgdatadir)/actions && \
	ln -sf action.sloppy-focus action

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/actions/action

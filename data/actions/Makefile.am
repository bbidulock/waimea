actionsdir = $(pkgdatadir)/actions
actions_DATA = action.sloppy-focus \
               action.click-to-focus \
               action.non-opaque \
               action.no-mouse

install-data-local:
	cd $(pkgdatadir)/actions && \
	ln -sf action.sloppy-focus action

uninstall-local:
	rm -f $(pkgdatadir)/actions/action

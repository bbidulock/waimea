DEFAULT_MENU=$(pkgdatadir)/menu
DEFAULT_STYLE=$(pkgdatadir)/styles/Default.style
DEFAULT_ACTION=$(pkgdatadir)/actions/action
DEFAULT_RCFILE=$(pkgdatadir)/config
DEFAULT_SCRIPT_DIR=$(pkgdatadir)/scripts

CPPFLAGS= @CPPFLAGS@ @SHAPE@ @XRENDER@ @XFT@ @INTERLACE@ @ORDEREDPSEUDO@ @PIXMAP@ \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTACTION=\"$(DEFAULT_ACTION)\" \
-DDEFAULTRCFILE=\"$(DEFAULT_RCFILE)\" \
-DDEFAULTSCRIPTDIR=\"$(DEFAULT_SCRIPT_DIR)\"

bin_PROGRAMS = waimea
waimea_SOURCES = main.cc \
		 Waimea.cc \
		 Screen.cc \
		 Window.cc \
		 Resources.cc \
		 Event.cc \
		 Image.cc \
		 Menu.cc \
		 Net.cc \
		 Dockapp.cc \
		 Timer.cc \
		 Regex.cc \
		 Font.cc

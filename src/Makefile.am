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
		 WaScreen.cc \
		 WaWindow.cc \
		 ResourceHandler.cc \
		 EventHandler.cc \
		 WaImage.cc \
		 WaMenu.cc \
		 NetHandler.cc \
		 DockappHandler.cc \
		 Timer.cc \
		 Regex.cc

DEFAULT_MENU=$(pkgdatadir)/menu
DEFAULT_STYLE=$(pkgdatadir)/styles/Default
DEFAULT_ACTION=$(pkgdatadir)/action

CPPFLAGS= @CPPFLAGS@ @XFT@ @INTERLACE@ @ORDEREDPSEUDO@ \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTACTION=\"$(DEFAULT_ACTION)\"

bin_PROGRAMS = waimea
waimea_SOURCES = main.cc \
		 Waimea.cc \
		 WaScreen.cc \
		 WaWindow.cc \
		 ResourceHandler.cc \
		 EventHandler.cc \
		 WaImage.cc \
		 WaMenu.cc \
		 NetHandler.cc

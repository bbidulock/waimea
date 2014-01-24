
bin_PROGRAMS = waimea

waimea_CPPFLAGS = \
		-include config.h \
		-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
		-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
		-DDEFAULTACTION=\"$(DEFAULT_ACTION)\" \
		-DDEFAULTRCFILE=\"$(DEFAULT_RCFILE)\" \
		-DDEFAULTSCRIPTDIR=\"$(DEFAULT_SCRIPT_DIR)\"
waimea_CXXFLAGS = \
		$(X11_CFLAGS) \
		$(XFT_CFLAGS) \
		$(RANDR_CFLAGS) \
		$(RENDER_CFLAGS) \
		$(SHAPE_CFLAGS) \
		$(XINERAMA_CFLAGS) \
		$(IMLIB2_CFLAGS)
waimea_SOURCES = \
		main.cc \
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
waimea_LDADD = \
		$(IMLIB2_LIBS) \
		$(XINERAMA_LIBS) \
		$(SHAPE_LIBS) \
		$(RENDER_LIBS) \
		$(RANDR_LIBS) \
		$(XFT_LIBS) \
		$(X11_LIBS)

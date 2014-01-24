dnl configure.in for Waimea

AC_PREREQ([2.69])
AC_INIT([waimea],[0.5.0],[http://github.com/bbidulock/waimea/issues])
AC_REVISION([0.5.0])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS

# Initialize Automake
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AM_MAINTAINER_MODE

# Checks for programs
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_INSTALL
AC_PROG_SED
AC_ARG_VAR([LD], [Linker loader command])

PKG_PROG_PKG_CONFIG

# Checks for libraries

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h])
AC_CHECK_HEADERS(ctype.h libgen.h signal.h stdio.h time.h unistd.h sys/select.h sys/signal.h sys/stat.h sys/time.h sys/types.h sys/wait.h regex.h)
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2 gettimeofday memset putenv regcomp strcasecmp strchr strncasecmp strstr])

PKG_CHECK_MODULES([X11],[x11])

dnl Check for Interlacing
AC_ARG_ENABLE([interlace],
	AC_HELP_STRING([--disable-interlace],
		[Include code for image interlacing
		@<:@default=excluded@:>@]))
AC_MSG_CHECKING([for interlacing images])
if test "x$enable_interlace" = xyes ; then
	AC_DEFINE_UNQUOTED([INTERLACE],[],[Define to include image interlacing code.])
fi
AC_MSG_RESULT([${enable_interlace:-no}])

dnl Check for ordered 8bpp dithering
AC_ARG_ENABLE([ordered-pseudo],
	AC_HELP_STRING([--disable-ordered-pseudo],
		[Include code for ordered pseudocolor (8bpp) dithering
		@<:@default=excluded@:>@]))
AC_MSG_CHECKING([for ordered pseudocolor (8bpp)])
if test "x$enable_ordered_pseudo" = xyes ; then
	AC_DEFINE_UNQUOTED([ORDEREDPSEUDO],[],[Define to include ordered pseudocolor
			(8bpp) dithering.])
fi
AC_MSG_RESULT([${enabled_ordered_pseudo:-no}])

dnl Check for XShape extension support
AC_ARG_ENABLE([shape],
	AC_HELP_STRING([--disable-shape],
		[Disable SHAPE support @<:@default=auto@:>@]))
if test "x$enable_shape" != xno ; then
	PKG_CHECK_MODULES([SHAPE],[xext],
		[AC_DEFINE_UNQUOTED([SHAPE], [], [Define to support SHAPE extension.])],
		[enable_shape=no])
fi
AC_MSG_CHECKING([for SHAPE support])
AC_MSG_RESULT([${enable_shape:-yes}])

dnl Check for Xinerama extension support
AC_ARG_ENABLE([xinerama],
	AC_HELP_STRING([--disable-xinerama],
		[Disable XINERAMA support @<:@default=auto@:>@]))
if test "x$enable_xinerama" != xno ; then
	PKG_CHECK_MODULES([XINERAMA],[xinerama],
		[AC_DEFINE_UNQUOTED([XINERAMA],[], [Define to support XINERAMA extension.])],
		[enable_xinerama=no])
fi
AC_MSG_CHECKING([for XINERAMA support])
AC_MSG_RESULT([${enable_xinerama:-yes}])

dnl Checks for Xrender extension support
AC_ARG_ENABLE([render],
	AC_HELP_STRING([--disable-render],
		[Disable RENDER support @<:@default=auto@:>@]))
if test "x$enable_render" != xno ; then
	PKG_CHECK_MODULES([RENDER],[xrender],
		[AC_DEFINE_UNQUOTED([RENDER],[],[Define to support RENDER extension.])],
		[enable_render=no])
fi
AC_MSG_CHECKING([for RENDER support])
AC_MSG_RESULT([${enable_render:-yes}])

dnl Check for Xrandr extension support
AC_ARG_ENABLE([randr],
	AC_HELP_STRING([--disable-randr],
		[Disable XRANDR support @<:@default=auto@:>@]))
if test "x$enable_randr" != xno ; then
	PKG_CHECK_MODULES([RANDR],[xrandr],
		[AC_DEFINE_UNQUOTED([RANDR],[], [Define to support RANDR extension.])],
		[enable_randr=no])
fi
AC_MSG_CHECKING([for RANDR support])
AC_MSG_RESULT([${enable_randr:-yes}])

dnl Checks for Xft library support
AC_ARG_ENABLE([xft],
	AC_HELP_STRING([--disable-xft],
		[Disable XFT support @<:@default=auto@:>@]))
if test "x$enable_xft" != xno ; then
	PKG_CHECK_MODULES([XFT],[xft],
		[AC_DEFINE_UNQUOTED([XFT],[], [Define to support XFT library.])],
		[enable_xft=no])
fi
AC_MSG_CHECKING([for XFT support])
AC_MSG_RESULT([${enable_xft:-yes}])

dnl Check for pixmap support
AC_ARG_ENABLE([pixmap],
	AC_HELP_STRING([--disable-pixmap],
		[Disable pixmap support @<:@default=auto@:>@]))
if test "x$enable_pixmap" != xno ; then
	PKG_CHECK_MODULES([IMLIB2],[imlib2],
		[AC_DEFINE_UNQUOTED([PIXMAP],[], [Define to support pixmap textures.])],
		[enable_pixmap=no])
fi
AC_MSG_CHECKING([for PIXMAP support])
AC_MSG_RESULT([${enable_pixmap:-yes}])

DEFAULT_MENU='$(pkgdatadir)/menu'
DEFAULT_STYLE='$(pkgdatadir)/styles/Default.style'
DEFAULT_ACTION='$(pkgdatadir)/actions/action'
DEFAULT_RCFILE='$(pkgdatadir)/config'
DEFAULT_SCRIPT='$(pkgdatadir)/scripts'

AC_SUBST([DEFAULT_MENU])
AC_SUBST([DEFAULT_STYLE])
AC_SUBST([DEFAULT_ACTION])
AC_SUBST([DEFAULT_RCFILE])
AC_SUBST([DEFAULT_SCRIPT])

test "x$prefix" = xNONE && prefix="$ac_default_prefix"
pkgdatadir="$datadir/waimea"
eval "pkgdatadir=\"$pkgdatadir\""
eval "pkgdatadir=\"$pkgdatadir\""
eval "pkgdatadir=\"$pkgdatadir\""

AC_SUBST([pkgdatadir])

AC_OUTPUT([Makefile
	   doc/waimea.1
	   data/Makefile
	   data/config
	   data/styles/Default.style
	   data/scripts/stylesdir.pl
	   src/Makefile])

AC_MSG_NOTICE([

  interlace       ${enable_interlace:-no}
  ordered-pseudo  ${enable_ordered_pseudo:-no}
  shape           ${enable_shape:-yes}
  xinerama        ${enable_xinerama:-yes}
  randr           ${enable_randr:-yes}
  render          ${enable_render:-yes}
  xft             ${enable_xft:-yes}
  pixmap          ${enable_pixmap:-yes}

  type 'make' to build Waimea.
])


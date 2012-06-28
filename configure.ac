AC_INIT([Robie C], [0.5], [anej001@aucklanduni.ac.nz],
             [robie_c], [])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 -Wall no-define])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AM_PROG_CC_C_O
PKG_CHECK_MODULES(libxml, libxml-2.0)
AC_SUBST(libxml_CFLAGS)
AC_SUBST(libxml_LIBS)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

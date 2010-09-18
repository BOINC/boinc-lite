#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(BoincLite, 0.0.1)
AM_INIT_AUTOMAKE(@PACKAGE_NAME@, @PACKAGE_VERSION@)
AC_CONFIG_SRCDIR([include/BoincLite.h])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([curl], [curl_version_info],,AC_MSG_ERROR([libcurl is needed]))
AC_CHECK_LIB([expat], [XML_ParserCreate],,AC_MSG_ERROR([libexpat is needed]))

# Checks for header files.
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])
AC_HEADER_DIRENT

AC_FUNC_CLOSEDIR_VOID
AC_CHECK_FUNCS([rmdir])
AC_CHECK_FUNCS([bzero])
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_TYPE_SIZE_T
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])
AC_TYPE_MODE_T
AC_TYPE_SIGNAL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

#

AC_ARG_ENABLE(debug,
  [  --enable-debug          enable debugging (default=no)],
  ENABLE_DEBUG=yes)

case "$ENABLE_DEBUG" in
yes)
  CFLAGS="-g -O0 -Wall"
  AC_DEFINE(DEBUG, 1, [Define to activate debugging message]) 
  ;;
*)
  CFLAGS="-O2 -Wall"
  AC_DEFINE(DEBUG, 0, [Define to activate debugging message])
  ;;
esac



AC_ARG_ENABLE(gc,
  [  --enable-gc          enable garbage collection (default=no)],
  ENABLE_GC=yes)

case "$ENABLE_GC" in
yes)
  AC_DEFINE(WITH_GC, 1, [Define to activate garbage collection])
  AC_CHECK_LIB([sgc], [sgc_run],,AC_MSG_ERROR([libsgc is needed]))
  ;;
*)
  AC_DEFINE(WITH_GC, 0, [Define to activate garbage collection])
  ;;
esac

# Conditions

TARGET=$host_os
AC_SUBST(TARGET)

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 example/Makefile
		 test/Makefile
		 include/Makefile])
AC_OUTPUT

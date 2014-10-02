AC_INIT([shmacro], [0.1])

dnl Uncomment if a specific autoconf version is required
dnl AC_PREREQ([2.68])

dnl Specify the header file that will contain the configuration #define's
AC_CONFIG_HEADERS([config.h])

dnl Move autotool output into subdirectories
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

dnl Use automake for makefiles
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects])
AM_SILENT_RULES([yes])

AC_CONFIG_FILES([Makefile])

dnl libtool is used to for shared libraries
LT_INIT

dnl Obtain a C compiler from the environment
AC_PROG_CC

dnl Allow an alternate libfabric installation to be easily specified via the
dnl 'ax_use_library' macro from this package. This will add the library and
dnl include paths to LDFLAGS and CPPFLAGS via the 'ax_append_flag' macro from
dnl the 'Autoconf Archive'. Additionally, an alternative library name for
dnl libfabric can also be specified. Subsequent configure tests should use
dnl the variable '$fabric_library_name', instead of directly using the static
dnl name 'fabric', to honor this configure option result.
AX_USE_LIBRARY([fabric])

dnl Sanity check for libfabric library
AC_SEARCH_LIBS(fi_getinfo, $fabric_library_name,
  [],
  [AC_MSG_FAILURE([cannot find libfabric library])])

dnl Sanity check for libfabric headers
AC_CHECK_HEADER(rdma/fabric.h,
  [],
  [AC_MSG_FAILURE([cannot find libfabric header])])

dnl Allow an alternate libpmi installation to be easily specified via the
dnl 'ax_use_library' macro from this package. This will add the library and
dnl include paths to LDFLAGS and CPPFLAGS via the 'ax_append_flag' macro from
dnl the 'Autoconf Archive'. Additionally, an alternative library name for
dnl libpmi can also be specified. Subsequent configure tests should use
dnl the variable '$pmi_library_name', instead of directly using the static
dnl name 'pmi', to honor this configure option result.
AX_USE_LIBRARY([pmi])

dnl Select the process manager implementation to use
AC_ARG_WITH([pm],
  [AS_HELP_STRING([--with-pm],
    [Specify the process manager to use (default=pmi)])],
  [build_pm=$withval],
  [build_pm=pmi]) dnl if not set, use 'pmi' implementation

dnl Sanity check for pmi library and headers
AS_IF([test x$build_pm = xpmi],
  [AC_SEARCH_LIBS(PMI_Init, $pmi_library_name,
    [],
    [AC_MSG_FAILURE([cannot find pmi library])])])

#AS_IF([test x$build_pm = xpmi],
#  [AC_CHECK_HEADER(pmi.h,
#    [],
#    [AC_MSG_FAILURE([cannot find pmi header])])])


AM_CONDITIONAL([BUILD_PM_PMI], [test x$build_pm = xpmi])


AC_OUTPUT

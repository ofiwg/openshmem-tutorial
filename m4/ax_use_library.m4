dnl
dnl AX_USE_LIBRARY([LIBRARY])
dnl
dnl This macro defines several configure options to customize the use of a
dnl particular library.
dnl
dnl An alternative installation of the library can be specified by using the
dnl '--with-LIBRARY' option.
dnl
dnl An alternative directory that contains the library file can be specified
dnl by using this '--with-LIBRARY-lib' option.
dnl
dnl An alternative directory that contains the library header file(s) can be
dnl specified by using this '--with-LIBRARY-include' option.
dnl
dnl Finally, and alternative _name_ for the library file can be specified by
dnl using the '--with-LIBRARY-name' option. This will set the variable
dnl 'LIBRARY_library_name' which can be used in subsequent configure tests.
dnl
AC_DEFUN([AX_USE_LIBRARY],
[
  AC_ARG_WITH([$1],
    [AS_HELP_STRING([--with-$1],
      [installation of $1])],
    [AX_APPEND_FLAG([-L${withval}/lib  -Wl,-rpath -Wl,${withval}/lib], [LDFLAGS])
     AX_APPEND_FLAG([-I${withval}/include], [CPPFLAGS])] dnl if-set
  )

  AC_ARG_WITH([$1-lib],
    [AS_HELP_STRING([  --with-$1-lib],
      [directory that contains the $1 library])],
    [AX_APPEND_FLAG([-L${withval}  -Wl,-rpath -Wl,${withval}], [LDFLAGS])] dnl if-set
  )

  AC_ARG_WITH([$1-include],
    [AS_HELP_STRING([  --with-$1-include],
      [directory that contains the $1 header file(s)])],
    [AX_APPEND_FLAG([-I${withval}], [CPPFLAGS])] dnl if-set
  )

  AC_ARG_WITH([$1-name],
    [AS_HELP_STRING([  --with-$1-name],
      [libary name to use instead of '$1'])],
    [$1_library_name=$withval], dnl if-set
    [$1_library_name=$1] dnl if-not-set
  )
]
) dnl AX_USE_LIBRARY





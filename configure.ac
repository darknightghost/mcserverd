#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mcserverd], [0.0.1], [darknightghost.cn@gmail.com])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE(mcserverd,0.0.1,darknightghost.cn@gmail.com)
AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([ssh], [ssh_new])
AC_CHECK_LIB([readline], [readline])
AC_CHECK_LIB([curses], [init_color])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h \
					stdlib.h \
					string.h \
					unistd.h \
					stdio.h \
					libssh/libssh.h \
					libssh/server.h \
					libssh/callbacks.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_OUTPUT([Makefile \
			conf/Makefile \
			src/Makefile])

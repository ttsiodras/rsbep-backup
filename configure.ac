AC_INIT([rsbep], [0.1.0-ttsiodras], [ttsiodras@gmail.com])

AC_CONFIG_MACRO_DIR([ac-macros])

# Don't clutter this dir, store in build-aux
AC_CONFIG_AUX_DIR([build-aux])

# Detect the canonical host and target build environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([-Wall -Werror])

AC_PROG_CC

AC_LANG(C)

# Stuff provided by autoscan
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([memset])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])
AC_C_INLINE
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_PREREQ

AM_PROG_AS
AM_PROG_CC_C_O

AX_CFLAGS_GCC_OPTION(-Wall)

AC_MSG_CHECKING(for native x86 asm support)
if AC_TRY_COMMAND($CC -c src/rs32asm.s) >/dev/null 2>&1; then
	X86ASM=yes
else
	X86ASM=no
fi
AC_MSG_RESULT($X86ASM)
AM_CONDITIONAL([HAVE_X86ASM], [test x$X86ASM = xyes])
if test x$X86ASM != xyes; then
	AC_MSG_WARN([*** Can't use native x86 asm version... it will be slow...])
fi

# Check for Python FUSE support
AC_PYTHON_MODULE(fuse)
if test x$HAVE_PYMOD_FUSE != xyes; then
	AC_MSG_WARN([*** Python FUSE was not found so you won't be able to mount "shielded" filesystems.])
fi

# Finally create all the generated files
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

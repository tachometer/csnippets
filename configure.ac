dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.50])
# TODO: use git if it's a git version, 0.1 for now....
AC_INIT([csnippets], [0.1])
AM_INIT_AUTOMAKE([1.10 foreign silent-rules])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/csnippets.h])
AM_CONFIG_HEADER([src/platform.h])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_LANG(C)

OPTIONAL_FLAGS=""
DEBUG_FLAGS="-O0 -fno-stack-protector -march=native -fomit-frame-pointer"

# check if we want select enabled
select_handler=no
AC_ARG_ENABLE(select-handler, [  --enable-socket-select		enable select interface],
	[OPTIONAL_FLAGS="$OPTIONAL_FLAGS -D__use_select"]
	select_handler=yes
)

# check if we want a debug build
debug_build=no
AC_ARG_ENABLE(debug, [  --enable-debug		enable debugging],
	[DEBUG_FLAGS="-D__debug_socket -D__debug_events -D__debug_list -O0 -g3"]
	debug_build=yes
)
AC_SUBST(DEBUG_FLAGS)

# check if we want the profiler
AC_ARG_ENABLE(profiler, [  --enable-profiler		enable profiler support], [PROFILER_FLAGS=-pg])
AC_SUBST(PROFILER_FLAGS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stddef.h ctype.h stdint.h stdlib.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostbyname gethostname memset strcasecmp strncasecmp strstr strtol])

# check for pthread
AC_CHECK_HEADERS([pthread.h], ,[AC_MSG_ERROR("pthread header not found.")])
AC_CHECK_LIB(pthread, main, ,[AC_MSG_ERROR("Linking against pthread library failed")])

if test "select_handler" = "no" ; then
    AC_CHECK_HEADERS([sys/epoll.h], ,[AC_MSG_ERROR("sys/epoll not found")])
    OPTIONAL_FLAGS="$OPTIONAL_FLAGS -D__use_epoll"
fi

AC_SUBST(OPTIONAL_FLAGS)

AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT

echo
echo $PACKAGE $VERSION
echo
echo Select handler.............. : $select_handler
echo Debug build................. : $debug_build
echo

echo Configure complete, now you may type \'./build.sh\'.


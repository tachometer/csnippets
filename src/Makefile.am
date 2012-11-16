lib_LIBRARIES = libcsnippets.a

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
CFLAGS      = -g -pipe -Wall -Wno-unused-parameter -Wno-sign-compare -Wextra \
	      -Wfatal-errors -I"../csnippets" -include csnippets.h ${DEFINES}

AM_CFLAGS = $(DEBUG_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT 

pkginclude_HEADERS = ../csnippets/socket.h ../csnippets/strmisc.h ../csnippets/atomic.h ../csnippets/rwlock.h \
			 ../csnippets/asprintf.h ../csnippets/list.h ../csnippets/event.h ../csnippets/task.h \
			 ../csnippets/map.h ../csnippets/config.h

libcsnippets_a_SOURCES = socket_select.c socket_epoll.c socket.c strmisc.c map.c error.c \
			  config.c rwlock.c asprintf.c list.c event.c task.c


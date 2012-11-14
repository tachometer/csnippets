noinst_LIBRARIES = libcsnippets.a
libcsnippets_adir = .

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
CFLAGS      = -g -pipe -Wall -Wno-unused-parameter -Wno-sign-compare -Wextra \
	      -Wfatal-errors -include ../src/util.h ${DEFINES}

AM_CFLAGS = $(DEBUG_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall -Wextra -Wno-strict-aliasing -Wfatal-errors 

libcsnippets_a_HEADERS = socket.h strmisc.h atomic.h rwlock.h asprintf.h list.h event.h task.h task.h

libcsnippets_a_SOURCES = socket_select.c socket_epoll.c socket.c strmisc.c map.c error.c \
			  config.c rwlock.c asprintf.c list.c event.c task.c


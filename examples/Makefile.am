noinst_PROGRAMS = tests

tests_SOURCES = tests.c
tests_LDADD = -L"../src" -lcsnippets

DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
CFLAGS      = -g -pipe -Wall -Wno-unused-parameter -Wno-sign-compare -Wextra \
	      -Wfatal-errors -include ../src/util.h ${DEFINES}

AM_CFLAGS =$(DEBUG_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall -Wextra -Wno-strict-aliasing -Wfatal-errors

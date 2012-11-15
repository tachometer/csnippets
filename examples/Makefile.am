noinst_PROGRAMS = client server stack tasks

client_SOURCES = client.c
server_SOURCES = server.c
stack_SOURCES = stack.c
tasks_SOURCES = tasks.c

LIBS += -L"../src/" -lcsnippets

DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
CFLAGS      = -g -pipe -Wall -Wno-unused-parameter -Wno-sign-compare -Wextra \
	      -Wfatal-errors -I"../csnippets" -include csnippets.h ${DEFINES}

AM_CFLAGS =$(DEBUG_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT

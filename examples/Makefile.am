noinst_PROGRAMS = client server stack tasks

THE_LIB = -L"../src/" -lcsnippets

client_SOURCES = client.c
client_LDADD = $(THE_LIB)

server_SOURCES = server.c
server_LDADD = $(THE_LIB)

stack_SOURCES = stack.c
stack_LDADD = $(THE_LIB)

tasks_SOURCES = tasks.c
tasks_LDADD = $(THE_LIB)

DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
CFLAGS      = -g -pipe -Wall -Wno-unused-parameter -Wno-sign-compare -Wextra \
	      -Wfatal-errors -include ../src/util.h ${DEFINES}

AM_CFLAGS =$(DEBUG_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall -Wextra -Wno-strict-aliasing -Wfatal-errors

CC = gcc
AR = ar rcs

prefix = /usr/local
bindir = ${prefix}/bin
hdrdir = ${prefix}/include

ifndef DEBUG_FLAGS
    DEBUG_FLAGS = -g -O0
endif
# In order to use select instead of epoll add -D__use_select to the list below
DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
INCLUDE_DIR = src
CFLAGS      = ${DEBUG_FLAGS} -I${INCLUDE_DIR} -Wall -Wno-unused-parameter \
	      -Wno-sign-compare -Wextra -Wfatal-errors  ${DEFINES}
LIBS        = -lpthread
LDFLAGS     = ${LIBS}

ifdef WIN32
    LIBS += -lws2_32
endif

BIN_DIR = bin
BIN     = a
LIB_DIR = lib
LIB     = libcutil.a
LIBS   += $(LIB_DIR)/$(LIB)

SRC_DIR = src
SRC     = socket_select.c socket_epoll.c socket.c strmisc.c map.c error.c config.c \
	  rwlock.c asprintf.c list.c event.c net.c
TEST    = tests
OBJ_DIR = obj
OBJ     = ${SRC:%.c=${OBJ_DIR}/%.o}
PRE     = pre.h

ifdef V
define compile
	$(CC) -c $(CFLAGS) $1 -o $@ $<
endef
else
define compile
	@echo Compiling $<
	@$(CC) -c $(CFLAGS) $1 -o $@ $<
endef
endif

define link
	@echo Linking executable
	$(CC) ${OBJ_DIR}/${TEST}.o -o $@  $(LDFLAGS)
endef

define ar
	@echo Linking library
	${AR} $@ ${OBJ}
endef

all: ${LIB_DIR}/${LIB} ${BIN_DIR}/$(BIN)
clean:
	$(RM) ${OBJ_DIR}/*.o ${OBJ_DIR}/*.gch ${BIN_DIR}/$(BIN)
	${RM} ${LIB_DIR}/${LIB}

install: ${BIN_DIR}/${BIN} ${SRC_DIR}/${SRC}
	install -d ${bindir}
	install -m 755 ${BIN_DIR}/${BIN} ${bindir}
	cp ${INCLUDE_DIR}/*.h ${hdrdir}

uninstall: ${bindir}
	${RM} ${bindir}/${BIN}

${OBJ_DIR}/pre.h.gch: ${SRC_DIR}/${PRE}
	${compile}

${LIB_DIR}/${LIB}: ${LIB_DIR} ${OBJ_DIR} ${OBJ}
	${ar}

${BIN_DIR}/${BIN}: ${LIB_DIR}/${LIB} ${OBJ_DIR}/${TEST}.o
	${link}

${OBJ_DIR}/${TEST}.o: ${SRC_DIR}/${TEST}.c ${OBJ_DIR}
	${compile}

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c ${OBJ_DIR}/${PRE}.gch
	${call compile, -include $(PRE)}

${OBJ_DIR}:
	@mkdir -p ${OBJ_DIR}
${BIN_DIR}:
	@mkdir -p ${BIN_DIR}
${LIB_DIR}:
	@mkdir -p ${LIB_DIR}

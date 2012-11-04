CC = gcc

prefix = /usr/local
bindir = ${prefix}/bin
hdrdir = ${prefix}/include

ifndef FLAGS
    FLAGS = -g -O0
endif
# In order to use select instead of epoll add -D__use_select to the list below
DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events -D__debug_socket
INCLUDE_DIR = src
CFLAGS      = ${FLAGS} -I${INCLUDE_DIR} -fstack-protector -Wall -Wno-unused-parameter -Wno-unused-but-set-variable \
	      -Wno-sign-compare -Wextra -Wfatal-errors -Wno-return-type ${DEFINES}
LIBS        = -lpthread

BIN_DIR = bin
BIN     = a

SRC_DIR = src
SRC     = socket_select.c socket_epoll.c socket.c strmisc.c map.c error.c config.c \
	  rwlock.c asprintf.c list.c event.c tests.c
OBJ_DIR = obj
OBJ     = $(SRC:%.c=${OBJ_DIR}/%.o)
PRE     = pre.h

ifdef VERBOSE
define compile
	$(CC) -c $(CFLAGS) $1 -o $@ $<
endef
else
define compile
	@echo " CC  $@"
	@$(CC) -c $(CFLAGS) $1 -o $@ $<
endef
endif

define link
	@echo " LD  $@"
	@$(CC) $(LIBS) $(CFLAGS) -o $@ $(OBJ)
endef

all: ${BIN_DIR}/$(BIN)
clean:
	$(RM) ${OBJ_DIR}/*.o ${OBJ_DIR}/*.gch ${BIN_DIR}/$(BIN)

install: ${BIN_DIR}/${BIN} ${SRC_DIR}/${SRC}
	install -d ${bindir}
	install -m 755 ${BIN_DIR}/${BIN} ${bindir}
	cp ${INCLUDE_DIR}/*.h ${hdrdir}

uninstall: ${bindir}
	# TODO: Remove headers too
	${RM} ${bindir}/${BIN}

${OBJ_DIR}/pre.h.gch: ${SRC_DIR}/${PRE}
	${compile}

${BIN_DIR}/${BIN}: ${BIN_DIR} ${OBJ_DIR} ${OBJ}
	${link}

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c ${OBJ_DIR}/${PRE}.gch
	${call compile, -include $(PRE)}

${OBJ_DIR}:
	@mkdir -p ${OBJ_DIR}
${BIN_DIR}:
	@mkdir -p ${BIN_DIR}


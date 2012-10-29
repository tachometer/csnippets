CC = gcc

ifndef FLAGS
    FLAGS = -g -O0
endif
DEFINES     = -D_GNU_SOURCE -D__debug_list -D__debug_events
INCLUDE_DIR = src
CFLAGS      = ${FLAGS} -I${INCLUDE_DIR} -fstack-protector -Wall -Wno-unused-parameter -Wno-unused-but-set-variable \
	      -Wno-sign-compare -Wextra -Wfatal-errors ${DEFINES}
LIBS        = -lpthread

ifdef rwtest
TEST  = rwlock_test.c
else
TEST  = tests.c
endif

BIN_DIR = bin
BIN     = a

SRC_DIR = src
SRC     = socket.c strmisc.c map.c error.c config.c rwlock.c asprintf.c list.c event.c ${TEST}
OBJ_DIR = obj
OBJ     = $(SRC:%.c=${OBJ_DIR}/%.o)
PRE     = pre.h

ifndef VERBOSE
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


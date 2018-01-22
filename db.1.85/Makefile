#	@(#)Makefile	8.9 (Berkeley) 7/14/94

include ../Rules.mk

LIBDB	= libdb.a

CFLAGS	+= -D__DBINTERFACE_PRIVATE

CFILES	:= db.c mpool.c
CFILES	+= bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c \
	bt_overflow.c bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c \
	bt_utils.c

OBJS	:= $(CFILES:.c=.o)

all:	${LIBDB}

${LIBDB}: ${OBJS}
	@rm -f $@
	${QUIET_AR}ar cqs $@ ${OBJS}
	${QUIET_LINK}ld -r -o db.1.85.o ${OBJS}

clean:
	rm -f ${LIBDB} ${OBJS} db.1.85.o
	$(MAKE) $(MFLAGS) -C test clean

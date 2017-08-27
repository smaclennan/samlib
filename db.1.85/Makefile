#	@(#)Makefile	8.9 (Berkeley) 7/14/94

LIBDB	= libdb.a

CFLAGS	+= -D__DBINTERFACE_PRIVATE -O2 -I. -Iinclude

CFILES	:= db.c mpool.c
CFILES	+= bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c \
	bt_overflow.c bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c \
	bt_utils.c

OBJS	:= $(CFILES:.c=.o)

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_LD      = $(Q:@=@echo    '     LD       '$@;)

.c.o:
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

all:	${LIBDB}

${LIBDB}: ${OBJS}
	@rm -f $@
	${QUIET_AR}ar cqs $@ ${OBJS}
	${QUIET_LD}ld -r -o db.1.85.o ${OBJS}

clean:
	rm -f ${LIBDB} ${OBJS} db.1.85.o
	make -C test clean

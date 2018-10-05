#	@(#)Makefile	8.9 (Berkeley) 7/14/94

include ../Rules.mk

CFLAGS	+= -D__DBINTERFACE_PRIVATE -I..

CFILES	:= db.c mpool.c
CFILES	+= bt_close.c bt_conv.c bt_delete.c bt_get.c bt_open.c \
	bt_overflow.c bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c \
	bt_utils.c

OBJS	:= $(addprefix $(BDIR)/, $(CFILES:.c=.o))

all:	$(BDIR) $(BDIR)/db.1.85.o

$(BDIR)/db.1.85.o: ${OBJS}
	${QUIET_LINK}${LD} -r -o $@ ${OBJS}

$(BDIR):
	@mkdir -p $(BDIR)

clean:
	rm -f ${LIBDB} *.o
	rm -f *.gcno *.gcda
	rm -rf $(BDIR)

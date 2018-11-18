
include ../Rules.mk

# Needed for freebsd
CFLAGS += -I.. -I/usr/local/include -L/usr/local/lib
# Needed for netbsd
CFLAGS += -I/usr/pkg/include

ifneq ($(findstring nto-qnx,$(shell $(CC) -dumpmachine)),)
LIBS += -lsocket
endif

SAMLIB = ../$(BDIR)/libsamlib.a
LIBS += $(SAMLIB)

ALL := ipaddr pkg serial udpping myps

all: $(ALL)

ipaddr: ipaddr.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< $(LIBS)

udpping: udpping.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< $(LIBS)

%: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< $(SAMLIB)

$(ALL): ../$(BDIR)/libsamlib.a ../samlib.h

clean:
	rm -f $(ALL)
	rm -f *.gcno *.gcda


CFLAGS += -Wall

include ../Rules.mk

ALL := ipaddr

all: $(ALL)

ipaddr: ipaddr.c
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $+ ../libsamlib.a

clean:
	rm -f $(ALL)

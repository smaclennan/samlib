
CFLAGS += -Wall

include ../Rules.mk

ALL := ipaddr find-dups imgsize  meminfo mkfilesz pkg prepro serial
ALL += strip-comments strip-comments2 udp-echo udpping

all: $(ALL)

ipaddr: ipaddr.c
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $+ ../libsamlib.a

find-dups: find-dups.c
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $+ ../libsamlib.a

imgsize: imgsize.c
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $+ -ljpeg -lgif

mkfilesz: mkfilesz.c
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $+ ../libsamlib.a

clean:
	rm -f $(ALL)

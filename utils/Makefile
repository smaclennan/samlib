
include ../Rules.mk

# Needed for freebsd
CFLAGS += -I.. -I/usr/local/include -L/usr/local/lib
# Needed for netbsd
CFLAGS += -I/usr/pkg/include

# For QNX
#LIBS += -lsocket

ALL := ipaddr find-dups imgsize  meminfo mkfilesz pkg prepro serial
ALL += strip-comments strip-comments2 udp-echo udpping md5sum

all: $(ALL)

ipaddr: ipaddr.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a $(LIBS)

udp-echo: udp-echo.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a $(LIBS)

udpping: udpping.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a $(LIBS)

find-dups: find-dups.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a

imgsize: imgsize.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< -ljpeg -lgif

mkfilesz: mkfilesz.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a

md5sum: md5sum.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a

%: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $<

$(ALL): ../libsamlib.a ../samlib.h

clean:
	rm -f $(ALL)

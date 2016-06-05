.PHONY: all check install clean

D = -O2
CFLAGS += -Wall $(D:1=-g)

LIBDIR ?= /usr/lib64

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)

.c.o:
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

CFILES := version.c readfile.c readcmd.c do-system.c walkfiles.c
CFILES += mkdir-p.c md5.c ip.c copy.c binary.c db.c

O := $(CFILES:.c=.o)

all: libsamlib.a stest

*.o: samlib.h

stest: stest.c libsamlib.a
	$(QUIET_CC)$(CC) -o $@ $+

libsamlib.a: $O
	$(QUIET_AR)$(AR) cr $@ $+

install:
	install -m644 -D samlib.h $(DESTDIR)/usr/include/samlib.h
	install -m644 -D linux-list.h $(DESTDIR)/usr/include/linux-list.h
	install -m644 -D libsamlib.a $(DESTDIR)$(LIBDIR)/libsamlib.a

clean:
	rm -f *.o libsamlib.a stest md5test

.PHONY: all check install clean

D = -O2
CFLAGS += -Wall $(D:1=-g)

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)

.c.o:
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

CFILES = readfile.c readcmd.c

O := $(CFILES:.c=.o)

all: libsamlib.a stest

stest: stest.c libsamlib.a
	$(QUIET_CC)$(CC) -o $@ $+

libsamlib.a: $O
	$(QUIET_AR)$(AR) cr $@ $+

clean:
	rm -f *.o libsamlib.a stest

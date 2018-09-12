# -*- mode: Makefile -*-
.PHONY: all check install clean

ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

# This works for gcc and clang
ifneq ($(shell $(CC) -v 2>&1 | fgrep "Target: x86_64"),)
AES ?= -maes
endif

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCDIR ?= $(PREFIX)/include

include ./Rules.mk

MFLAGS += CC=$(CC) LD=$(LD)

# Uncomment for system db rather than builtin
#CFLAGS += -DHAVE_DB_H
#DBLIB = -ldb
# And comment this out
EXTRA_LIB = db.1.85/libdb.a
EXTRA_OBJ = db.1.85/db.1.85.o

CFILES := version.c readfile.c readcmd.c do-system.c walkfiles.c
CFILES += mkdir-p.c md5.c ip.c copy.c binary.c samdb.c time.c
CFILES += arg-helpers.c xorshift.c must.c readproc.c base64.c
CFILES += crc16.c file.c dumpstack.c sha256.c aes128.c aes128-cbc.c
CFILES += tsc.c cpuid.c safecpy.c slackware.c is-elf.c

O := $(CFILES:.c=.o)

all: $(EXTRA) libsamlib.a libsamthread.a
	$(MAKE) $(MFLAGS) -C tests
	$(MAKE) $(MFLAGS) -C utils

libsamlib.a: $O $(EXTRA_LIB)
	$(QUIET_AR)$(AR) cr $@ $O $(EXTRA_OBJ)

db.1.85/libdb.a:
	$(QUIET_MAKE)$(MAKE) $(MFLAGS) -C db.1.85

*.o: samlib.h

aes128.o: aes128.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(AES) -c $< -o $@

aes128-cbc.o: aes128-cbc.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(AES) -c $< -o $@

# Threading - Linux x86 only
libsamthread.a: samthread.o
	$(QUIET_AR)$(AR) cr $@ samthread.o

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(INCDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)
	install -m644 linux-list.h $(DESTDIR)$(INCDIR)/linux-list.h
	install -m644 samlib.h $(DESTDIR)$(INCDIR)/samlib.h
	install -m644 libsamlib.a $(DESTDIR)$(LIBDIR)/libsamlib.a
	install -m644 samthread.h $(DESTDIR)$(INCDIR)/samthread.h
	install -m644 libsamthread.a $(DESTDIR)$(LIBDIR)/libsamthread.a
	install utils/ipaddr $(DESTDIR)$(BINDIR)/ipaddr
	install utils/imgsize $(DESTDIR)$(BINDIR)/imgsize

# sparse cannot handle aes128.c
check:
	sparse $(filter-out aes128.c,$(CFILES))

clean:
	rm -f *.o libsamlib.a libsamthread.a
	rm -f *.gcno *.gcda
	$(MAKE) $(MFLAGS) -C db.1.85 clean
	$(MAKE) $(MFLAGS) -C tests clean
	$(MAKE) $(MFLAGS) -C utils clean

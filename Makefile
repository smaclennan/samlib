# -*- mode: Makefile -*-
.PHONY: first-rule all check install clean

ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCDIR ?= $(PREFIX)/include

include ./Rules.mk

CFLAGS += -DWANT_FLOATS

ifeq ($(shell uname -s), QNX)
# We need gcc or clang for aes
CC = gcc
endif

# This works for gcc and clang
ifneq ($(shell $(CC) -v 2>&1 | fgrep "Target: x86_64"),)
AES ?= -maes
endif

MFLAGS += CC=$(CC) LD=$(LD) BDIR=$(BDIR) --no-print-directory

EXTRA_OBJ = db.1.85/$(BDIR)/db.1.85.o

CFILES := version.c readfile.c readcmd.c do-system.c walkfiles.c
CFILES += mkdir-p.c md5.c ip.c copy.c binary.c samdb.c time.c
CFILES += arg-helpers.c xorshift.c must.c readproc.c base64.c
CFILES += crc16.c file.c dumpstack.c sha256.c aes128.c aes-cbc.c
CFILES += tsc.c cpuid.c safecpy.c slackware.c is-elf.c globals.c
CFILES += strfmt.c socket.c tea.c strlcpy.c

O := $(addprefix $(BDIR)/, $(CFILES:.c=.o))

first-rule: $(BDIR) $(EXTRA) $(BDIR)/libsamlib.a $(BDIR)/libsamthread.a
ifeq ($(CROSS_COMPILE),)
	$(QUIET_MAKE)$(MAKE) $(MFLAGS) -C tests
endif

all:	first-rule
	$(QUIET_MAKE)$(MAKE) $(MFLAGS) -C tests

$(BDIR)/libsamlib.a: $O $(EXTRA_OBJ)
	$(QUIET_AR)$(AR) cr $@ $O $(EXTRA_OBJ)

db.1.85/$(BDIR)/db.1.85.o:
	$(QUIET_MAKE)$(MAKE) $(MFLAGS) -C db.1.85

$(BDIR):
	@mkdir -p $(BDIR)

*.o: samlib.h

$(BDIR)/aes128.o: aes128.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(AES) -c $< -o $@

$(BDIR)/aes-cbc.o: aes-cbc.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(AES) -c $< -o $@

# Threading - Linux only
$(BDIR)/libsamthread.a: $(BDIR)/samthread.o $(BDIR)/mutex.o
	$(QUIET_AR)$(AR) cr $@ $+

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(INCDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)
	install -m644 linux-list.h $(DESTDIR)$(INCDIR)/linux-list.h
	install -m644 samlib.h $(DESTDIR)$(INCDIR)/samlib.h
	install -m644 $(BDIR)/libsamlib.a $(DESTDIR)$(LIBDIR)/libsamlib.a
	install -m644 samthread.h $(DESTDIR)$(INCDIR)/samthread.h
	install -m644 $(BDIR)/libsamthread.a $(DESTDIR)$(LIBDIR)/libsamthread.a

# sparse cannot handle aes128*.c
check:
	sparse -D__linux__ $(filter-out aes128.c aes128-cbc.c, $(CFILES))

clean:
	rm -rf $(BDIR)
	rm -f *.gcno *.gcda
	$(MAKE) $(MFLAGS) -C db.1.85 clean
	$(MAKE) $(MFLAGS) -C tests clean

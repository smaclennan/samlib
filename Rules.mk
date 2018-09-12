#CC = clang -fno-color-diagnostics

BDIR ?= $(shell uname -m)

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_MAKE    = $(Q:@=@echo    '   MAKE       '$@;)

D = -O2
CFLAGS += $(D:1=-g -D__DEBUG__) -Wall

$(BDIR)/%.o : %.c
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

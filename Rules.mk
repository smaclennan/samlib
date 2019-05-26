#CC = clang -fno-color-diagnostics
#CC = gcc

ifeq ($(CROSS_COMPILE),)
BDIR ?= $(shell uname -m)
else
# uname -m really doesn't make sense for cross compiling
BDIR ?= $(ARCH)

CC := $(CROSS_COMPILE)$(CC)
LD := $(CROSS_COMPILE)$(LD)
endif

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

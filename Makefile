ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

MFLAGS += --no-print-directory

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_MAKE    = $(Q:@=@echo    '     MAKE     '$@;)

include Make.common

ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
else
LIBDIR ?= /usr/lib
endif

ifeq ($(shell uname -s), Linux)
DBLIB ?= -ldb
endif

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)

include Make.common

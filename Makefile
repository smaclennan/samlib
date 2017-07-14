ifneq ($(wildcard /usr/include/db.h),)
CFLAGS += -DHAVE_DB_H
DBLIB ?= -ldb
ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif
endif

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)

include Make.common

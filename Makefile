ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

MFLAGS += --no-print-directory

include make.common

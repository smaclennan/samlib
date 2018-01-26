ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

include make.common

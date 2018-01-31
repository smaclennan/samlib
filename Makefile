ifeq ($(shell uname -m), x86_64)
LIBDIR ?= /usr/lib64
endif

# This works for gcc and clang
ifneq ($(shell $(CC) -v 2>&1 | fgrep "Target: x86_64"),)
AES ?= -maes
endif

include make.common

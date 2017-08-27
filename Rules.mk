#CC = clang -fno-color-diagnostics

V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_MAKE    = $(Q:@=@echo    '     MAKE     '$@;)

D = -O2
CFLAGS += $(D:1=-g -D__DEBUG__)

.c.o:
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

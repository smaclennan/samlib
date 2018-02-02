include ../Rules.mk

#CC = clang -fno-color-diagnostics

# Uncomment if you want system db rather than builtin
#CFLAGS += -DHAVE_DB_H
#LIBS += -ldb

# Define for pthreads
#LIBS += -lpthread

TESTS := args base64 cptest crc16test md5test random readfile
TESTS += sha256test timetest threadtest spinlock dbtest
TESTS += readproctest aes-test aes-stress mmap-test mutex-timing
TESTS += testtsc

OTHERS := bitgen

all: $(TESTS) $(OTHERS) testall

testall: testall.c $(TESTS)

test: all
	for t in $(TESTS); do echo $$t; ./$$t; done

%: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a ../libsamthread.a $(LIBS)

$(TESTS): ../libsamlib.a ../samlib.h

clean:
	rm -f $(TESTS) $(OTHERS) testall
	rm -f TAGS readfile.txt cptest.txt

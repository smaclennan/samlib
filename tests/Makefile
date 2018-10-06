include ../Rules.mk

#CC = clang -fno-color-diagnostics

# Define for pthreads
#LIBS += -lpthread

# For gcov
#CFLAGS += -coverage

ifneq ($(wildcard /usr/include/valgrind/valgrind.h),)
CFLAGS += -DHAVE_VALGRIND_H
endif

TESTS := args base64 cptest crc16test md5test random readfile
TESTS += sha256test timetest threadtest spinlock dbtest
TESTS += readproctest aes-test aes-stress mutex-timing
TESTS += tsctest strtest

OTHERS := bitgen

all: $(TESTS) $(OTHERS) testall

testall: testall.c $(TESTS)

args: args.c ../arg-helpers.c
base64: base64.c ../base64.c
cptest: cptest.c ../copy.c
crc16test: crc16test.c ../crc16.c
md5test: md5test.c ../md5.c
random: random.c ../xorshift.c
readfile: readfile.c ../readfile.c
sha256test: sha256test.c ../sha256.c
timetest: timetest.c ../time.c
dbbtest: dbtest.c ../samdb.c ../db.1.85/$(BDIR)/db.1.85.o
readproctest: readproctest.c ../readproc.c
aes-test: aes-test.c ../aes128.c
aes-stress: aes-stress.c ../aes128.c
tsctest: tsctest.c ../tsc.c
strtest: strtest.c ../safecpy.c ../strfmt.c

threadtest: threadtest.c ../$(BDIR)/libsamthread.a
spinlock: spinlock.c ../$(BDIR)/libsamthread.a
mutex-timing: mutex-timing.c ../$(BDIR)/libsamthread.a

test: all
	for t in $(TESTS); do echo $$t; ./$$t; done

LIBS += ../$(BDIR)/libsamlib.a ../$(BDIR)/libsamthread.a

%: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ $< $(LIBS)

$(TESTS): ../samlib.h

clean:
	rm -f $(TESTS) $(OTHERS) testall
	rm -f *.gcno *.gcda *.gcov
	rm -f TAGS readfile.txt cptest.txt

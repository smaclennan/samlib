CFLAGS += -Wall -O2

# Undefine if you don't have db.h
CFLAGS += -DHAVE_DB_H
LIBS += -ldb

# Define for pthreads
LIBS += -lpthread

TESTS := args base64 cptest crc16test md5test random readfile
TESTS += sha256test timetest threadtest spinlock dbtest
TESTS += readproctest

OTHERS := bitgen

all: $(TESTS) $(OTHERS) testall

testall: testall.c $(TESTS)

test: all
	for t in $(TESTS); do echo $$t; ./$$t; done

%: %.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a ../libsamthread.a $(LIBS)

clean:
	rm -f $(TESTS) $(OTHERS) testall
	rm -f TAGS readfile.txt cptest.txt

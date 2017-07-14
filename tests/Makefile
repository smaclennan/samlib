CFLAGS += -Wall -O2

TESTS := args base64 cptest crc16test md5test random readfile
TESTS += sha256test timetest threadtest spinlock

OTHERS := bitgen

all: $(TESTS) $(OTHERS) testall

testall: testall.c $(TESTS)

test: all
	for t in $(TESTS); do echo $$t; ./$$t; done

%: %.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a ../libsamthread.a

clean:
	rm -f $(TESTS) $(OTHERS) testall
	rm -f TAGS readfile.txt cptest.txt

CFLAGS += -Wall -O2

TESTS := base64 cptest crc16test md5test proc+system random readfile
TESTS += sha256test threadtest

all: $(TESTS)

test: all
	for t in $(TESTS); do echo $$t; ./$$t; done

%: %.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a ../libsamthread.a

clean:
	rm -f $(TESTS)
	rm -f readfile.txt cptest.txt

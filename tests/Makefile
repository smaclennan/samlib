CFLAGS += -Wall -O2

all: base64 crc16test md5test random readfile sha256test

test: all
	./base64
	./crc16test
	./md5test
	./random
	./readfile
	./sha256test

random: random.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a ../libsamthread.a

%: %.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a

clean:
	rm -f base64 crc16test md5test random readfile sha256test
	rm -f readfile.txt

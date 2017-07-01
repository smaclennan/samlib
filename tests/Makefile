CFLAGS += -Wall -O2

all: base64 crc16test md5test readfile sha256test

test: all
	./base64
	./crc16test
	./md5test
	./readfile
	./sha256test

%: %.c
	$(CC) $(CFLAGS) -o $@ $< ../libsamlib.a

clean:
	rm -f base64 crc16test md5test readfile sha256test
	rm -f readfile.txt

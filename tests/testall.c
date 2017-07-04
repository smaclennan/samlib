#define TESTALL
#define exit(a) bogus(a)

#include "base64.c"
#include "cptest.c"
#include "crc16test.c"
#include "md5test.c"
// #include "random.c"
#include "readfile.c"
#include "sha256test.c"
#include "threadtest.c"


int main(int argc, char *argv[])
{
	int rc = 0;

	rc |= base64_main();
	rc |= cptest_main();
	rc |= crc16_main();
	rc |= md5_main();
	rc |= readfile_main();
	rc |= sha256_main();
	rc |= thread_main();

	return rc;
}

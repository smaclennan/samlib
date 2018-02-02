#define TESTALL
#define exit(a) bogus(a)

#include "args.c"
#include "base64.c"
#include "cptest.c"
#include "crc16test.c"
#include "dbtest.c"
#include "md5test.c"
// #include "random.c"
#include "readfile.c"
#include "readproctest.c"
#include "sha256test.c"
#include "testtsc.c"
#include "threadtest.c"
#include "timetest.c"
#include "spinlock.c"
#include "aes-test.c"
#include "mmap-test.c"


int main(int argc, char *argv[])
{
	int rc = 0;

	printf("Version %s\n", samlib_version);

	rc |= aes_main();
	rc |= args_main();
	rc |= base64_main();
	rc |= cptest_main();
	rc |= crc16_main();
	rc |= db_main();
	rc |= md5_main();
	rc |= mmap_main();
	rc |= readfile_main();
	rc |= readproc_main();
	rc |= sha256_main();
	rc |= tsc_main();
	rc |= spinlock_main();
	rc |= thread_main();
	rc |= time_main();

	if (rc == 0)
		puts("Success");
#ifdef WIN32
	printf("Hit return to exit...");  getchar();
#endif

	return rc;
}

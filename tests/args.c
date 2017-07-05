#include <stdio.h>
#include <string.h>
#include "../samlib.h"

#ifdef TESTALL
static int args_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	uint64_t size;
	unsigned result;
	unsigned long seconds;
	char ch;
	int rc = 0;
	char str[24];

	size = get_mem_len("2t");
	result = nice_mem_len(size, &ch);
	if (size != (2ULL << 40) || result != 2 || ch != 'T') {
		printf("Problems with nice_mem_len()\n");
		rc = 1;
	}

	seconds = get_duration("30d");
	nice_duration(seconds, str, sizeof(str));
	if (seconds != 2592000UL || strcmp(str, "30d")) {
		printf("Problems with 30d: %lu !=  2592000, %s != 30d\n",
			   seconds, str);
		rc = 1;
	}

	seconds = get_duration("1.2h");
	nice_duration(seconds, str, sizeof(str));
	if (seconds != 4320 || strcmp(str, "1h12m")) {
		printf("Problems with 1.2h: %lu != 4320, %s != 1h12m\n",
			   seconds, str);
		rc = 1;
	}

	if (strcmp(nice_number(100000000UL), "100,000,000")) {
		puts("Problems with nice_number");
		rc = 1;
	}

	return rc;
}

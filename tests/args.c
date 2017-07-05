#include <stdio.h>
#include "../samlib.h"

#ifdef TESTALL
static int args_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	uint64_t size;
	unsigned result;
	char ch;
	int rc = 0;

	size = 2ULL << 40;
	result = nice_mem_len(size, &ch);
	if (result != 2 || ch != 'T') {
		printf("Problems with nice_mem_len()\n");
		rc = 1;
	}

	return rc;
}

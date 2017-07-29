#include <stdio.h>

#include "samlib.h"

int main(int argc, char *argv[])
{
#if 0
	int c, flags = 0;

	while ((c = getopt(argc, argv, "f:vV")) != EOF)
		switch (c) {
		case 'v':
			flags |= WALK_VERBOSE;
			break;
		case 'f':
			flags |= strtol(optarg, NULL, 0);
			break;
		case 'V':
			printf("Version  '%s'\n", samlib_version);
			exit(0);
		}

	if (argc == optind) {
		fputs("I need a directory!\n", stderr);
		exit(1);
	}

	return walkfiles(NULL, argv[optind], NULL, 0);
#else
	char *ifaces[16];
	int i, n;


	n = get_interfaces(ifaces, 16, IFACE_UP);
	printf("Got %d up:\n", n);
	for (i = 0; i < n; ++i)
		puts(ifaces[i]);

	n = get_interfaces(ifaces, 16, IFACE_DOWN);
	printf("Got %d down:\n", n);
	for (i = 0; i < n; ++i)
		puts(ifaces[i]);

	n = get_interfaces(ifaces, 16, 0);
	printf("Got %d total:\n", n);
	for (i = 0; i < n; ++i)
		puts(ifaces[i]);

	return 0;
#endif
}

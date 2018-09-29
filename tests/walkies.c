#include "../samlib.h"

int main(int argc, char *argv[])
{
	char *dir;

	if (argc == 1)
		dir = ".";
	else
		dir = argv[1];

	walkfiles(NULL, dir, NULL, WALK_DOTFILES);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall walkies.c -o walkies ../x86_64/libsamlib.a"
 * End:
 */

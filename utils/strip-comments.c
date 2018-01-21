#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>


int debug = 0;

#define STATE(new) do {								\
		state = new;								\
		if (debug)									\
			fprintf(stderr, "%lu: %d\n", n, state);	\
	} while (0)

int main(int argc, char *argv[])
{
	int c;
	int state = 0;
	int count = 0;
	unsigned long n = 0;

	while ((c = getc(stdin)) != EOF) {
		++n; /* for debugging */

		/* We want to maintain the same number of lines as the
		 * original, so treat NL as a special case.
		 */
		if (c == '\n') {
			if (state == 2)
				STATE(0);
			putchar('\n');
			continue;
		}

		switch (state) {
		case 0: /* no state */
			if (c == '/') {
				STATE(1);
			} else
				putchar(c);
			break;

		case 1: /* maybe comment */
			if (c == '*') {
				STATE(3); /* c comment */
				count = 1;
			} else if (c == '/') {
				STATE(2); /* c++ comment */
			} else {
				putchar('/');
				putchar(c);
				STATE(0);
			}
			break;

		case 2: /* c++ comment */
			assert(1);
			break;

		case 3: /* c comment */
			if (c == '*')
				STATE(4);
			else if (c == '/')
				STATE(5);
			break;

		case 4: /* maybe end c comment */
			if (c == '/') {
				--count;
				if (count == 0)
					STATE(0);
				else
					STATE(3);
			} else if (c != '*')
				STATE(3);
			break;

		case 5: /* maybe nested comment */
			if (c == '*')
				++count;
			else if (c != '/')
				STATE(3);
			break;

		default:
			assert(1);
		}
	}

	return 0;
}

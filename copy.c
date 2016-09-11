#define USE_SENDFILE

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef USE_SENDFILE
#include <sys/sendfile.h>
#endif

/* Copy a file. Uses sendfile by default.
 * Returns number of bytes copied or < 0 on error:
 *    -1 = I/O error
 *    -2 = error on from
 *    -3 = error on to
 */
long copy_file(const char *from, const char *to)
{
	int in = open(from, O_RDONLY);
	if (in < 0)
		return -2;

	struct stat sbuf;
	if (fstat(in, &sbuf)) {
		close(in);
		return -2;
	}

	int out = creat(to, sbuf.st_mode);
	if (out < 0) {
		close(in);
		return -3;
	}

#ifdef USE_SENDFILE
	ssize_t n = sendfile(out, in, NULL, sbuf.st_size);
#else
	char buf[4096];
	int n, total = 0;
	while ((n = read(in, buf, sizeof(buf))) > 0) {
		int wrote = write(out, buf, n);
		if (wrote != n) {
			total = -1;
			break;
		}
		total += n;
	}
	n = total;
#endif

	close(in);
	close(out);

	return n;
}

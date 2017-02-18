#include <fcntl.h>
#include <errno.h>
#include "samlib.h"

/* Create a file of set length and mode. If mode is 0, a reasonable default is chosen. */
int create_file(const char *fname, off_t length, mode_t mode)
{
	if (access(fname, F_OK) == 0)
		return EEXIST;

	if (mode == 0)
		mode = 0644;

	int fd = creat(fname, mode);
	if (fd < 0)
		return errno;

	if (ftruncate(fd, length)) {
		close(fd);
		return errno;
	}

	close(fd);
	return 0;
}

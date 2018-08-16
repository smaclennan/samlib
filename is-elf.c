#include "samlib.h"
#include <fcntl.h>

static const uint8_t elf_hdr[] = { 0177, 'E', 'L', 'F' };

int is_elf(const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	uint8_t buff[sizeof(elf_hdr)];
	int n = read(fd, buff, sizeof(buff));
	close(fd);

	if (n == sizeof(elf_hdr))
		return !!memcmp(buff, elf_hdr, sizeof(elf_hdr));

	if (n >= 0)
		errno = EINVAL;

	return -1;
}

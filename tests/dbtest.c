#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_DB_H
#include <db.h>
#endif
#include "../samlib.h"

static int db_walk_strings(const char *key, void *data, int len)
{
	static char kstr[2] = "a", vstr[2] = "1";

	if (strcmp(key, kstr) || strcmp((char *)data, vstr)) {
		printf("'%s' '%s' != '%s' %s'\n", key, (char *)data, kstr, vstr);
		return 1; /* failed */
	}

	++*kstr;
	++*vstr;

	return 0;
}

char *create_tmp_file(void)
{
#ifdef WIN32
	static char path[100];
	DWORD n;

	n = GetTempPath(sizeof(path) - 8, path);
	if (n >= sizeof(path) - 8 || n == 0)
		_snprintf(path, sizeof(path), "c:/tmp/dbtest");
	else
		strcat(path, "dbtest");
	printf("tmpfile %s\n", path);
	return path;
#else
	return "/tmp/dbtest";
#endif
}

#ifdef TESTALL
int db_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	char str[8];
	int rc = 0;

	char *tmpfile = create_tmp_file();

	unlink(tmpfile);

	if (db_open(tmpfile, DB_CREATE, NULL)) {
		puts("Unable to open DB");
		return 1;
	}

	if (db_put_str(NULL, "c", "3") ||
		db_put_str(NULL, "b", "2") ||
		db_put_str(NULL, "a", "1")) {
		printf("Puts failed\n");
		rc = 1;
	}

	db_get_str(NULL, "b", str, sizeof(str));
	if (strcmp(str, "2")) {
		printf("Problem: expected 2 got %s\n", str);
		rc = 1;
	}

	if (db_walk(NULL, db_walk_strings))
		rc = 1;

	db_close(NULL);

	unlink(tmpfile);

	if (db_open(tmpfile, DB_CREATE, NULL)) {
		puts("Unable to open DB");
		return 1;
	}

	db_close(NULL);

	unlink(tmpfile);

	return rc;
}

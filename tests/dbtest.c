#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#ifdef HAVE_DB_H
#include <db.h>

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

static int db_walk_longs(const char *key, void *data, int len)
{
	static char kstr[2] = "a";
	static long v = 1;

	if (strcmp(key, kstr) || *(long *)data != v) {
		printf("'%s' %ld != '%s' %ld\n", key, *(long *)data, kstr, v);
		return 1; /* failed */
	}

	++*kstr;
	++v;

	return 0;
}

#ifdef TESTALL
int db_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	char str[8];
	int rc = 0;

	unlink("/tmp/dbtest");

	if (db_open("/tmp/dbtest", DB_CREATE, NULL)) {
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

	unlink("/tmp/dbtest");

	if (db_open("/tmp/dbtest", DB_CREATE, NULL)) {
		puts("Unable to open DB");
		return 1;
	}

	if (db_update_long(NULL, "c", 3) ||
		db_update_long(NULL, "b", 2) ||
		db_update_long(NULL, "a", 1)) {
		printf("Puts failed\n");
		rc = 1;
	}

	if (db_walk(NULL, db_walk_longs))
		rc = 1;

	db_close(NULL);

	unlink("/tmp/dbtest");

	return rc;
}
#else
#ifdef TESTALL
int db_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	puts("No db.h");
	return 0;
}
#endif

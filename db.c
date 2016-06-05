#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <db.h>

#include "samlib.h"


static DB *global_db;

int db_open(char *dbname, uint32_t flags)
{
	if (global_db)
		return -EBUSY;

	int rc = db_create(&global_db, NULL, 0);
	if (rc)
		return rc;

	rc = global_db->open(global_db, NULL, dbname, NULL, DB_BTREE, flags, 0664);
	if (rc)
		return rc;

	global_db = global_db;
	atexit(db_close);
	return 0;
}

void db_close(void)
{
	if (global_db) {
		global_db->close(global_db, 0);
		global_db = NULL;
		global_db = NULL;
	}
}

int db_add(char *keystr, void *val, int len)
{
	DBT key, data;

	if (!global_db)
		return -EINVAL;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = keystr;
	key.size = strlen(keystr) + 1;
	data.data = val;
	data.size = len;

	return global_db->put(global_db, NULL, &key, &data, 0);
}

int db_add_str(char *keystr, char *valstr)
{
	return db_add(keystr, valstr, strlen(valstr) + 1);
}

int db_get(char *keystr, void *val, int len)
{
	DBT key, data;

	if (!global_db)
		return -EINVAL;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = keystr;
	key.size = strlen(keystr) + 1;

	int rc = global_db->get(global_db, NULL, &key, &data, 0);
	if (rc)
		return -rc;

	if (len > data.size)
		len = data.size;
	memcpy(val, data.data, len);

	return len;
}

/* The difference between db_get() and db_get_str() is that
 * db_get_str() guarantees a null terminated string if the return > 0.
 */
int db_get_str(char *keystr, char *valstr, int len)
{
	int rc = db_get(keystr, valstr, len);
	if (rc <= 0)
		return rc;

	if (valstr[rc - 1] == 0)
		--rc; /* normal case */
	else if (rc == len)
		valstr[len - 1] = 0; /* truncated */
	else
		valstr[rc] = 0; /* no null in val */

	return rc;
}

int db_walk(int (*walk_func)(char *key, void *data, int len))
{
	DBT key, data;
	DBC *dbc;
	int rc = 0;

	if (!global_db)
		return -EINVAL;

	if (!walk_func)
		return -EINVAL;

	if ((rc = global_db->cursor(global_db, NULL, &dbc, 0))) {
		global_db->close(global_db, 0);
		return rc;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	while (dbc->c_get(dbc, &key, &data, DB_NEXT) == 0)
		if ((rc = walk_func(key.data, data.data, data.size)))
			break;

	dbc->c_close(dbc);

	return rc;
}

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <db.h>

#include "samlib.h"


static DB *global_db;

static void db_close_global(void)
{
	if (global_db) {
		global_db->close(global_db, 0);
		global_db = NULL;
	}
}

int db_open(char *dbname, uint32_t flags, void **dbh)
{
	DB *db;

	if (!dbh && global_db)
		return -EBUSY;

	int rc = db_create(&db, NULL, 0);
	if (rc)
		return rc;

	rc = db->open(db, NULL, dbname, NULL, DB_BTREE, flags, 0664);
	if (rc)
		return rc;

	if (dbh)
		*dbh = db;
	else {
		global_db = db;
		atexit(db_close_global);
	}
	return 0;
}

#define GET_DB(dbh)							\
	DB *db = (dbh) ? (dbh) : global_db;			\
	if (!db)									\
		return -EINVAL

int db_close(void *dbh)
{
	GET_DB(dbh);
	if (db == global_db)
		db_close_global();
	else
		db->close(db, 0);
	return 0;
}

int db_put(void *dbh, char *keystr, void *val, int len, unsigned flags)
{
	DBT key, data;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = keystr;
	key.size = strlen(keystr) + 1;
	data.data = val;
	data.size = len;

	return db->put(db, NULL, &key, &data, flags);
}

int db_put_str(void *dbh, char *keystr, char *valstr)
{
	return db_put(dbh, keystr, valstr, strlen(valstr) + 1, 0);
}

int db_get(void *dbh, char *keystr, void *val, int len)
{
	DBT key, data;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = keystr;
	key.size = strlen(keystr) + 1;

	int rc = db->get(db, NULL, &key, &data, 0);
	if (rc)
		return rc < 0 ? rc : -rc;

	if (len > data.size)
		len = data.size;
	memcpy(val, data.data, len);

	return len;
}

/* The difference between db_get() and db_get_str() is that
 * db_get_str() guarantees a null terminated string if the return > 0.
 */
int db_get_str(void *dbh, char *keystr, char *valstr, int len)
{
	int rc = db_get(dbh, keystr, valstr, len);
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

int db_peek(void *dbh, char *keystr)
{
	DBT key;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));

	key.data = keystr;
	key.size = strlen(keystr) + 1;

	return db->get(db, NULL, &key, NULL, 0);
}

int db_del(void *dbh, char *keystr)
{
	DBT key;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));

	key.data = keystr;
	key.size = strlen(key.data) + 1;

	return db->del(db, NULL, &key, 0);
}

int db_walk(void *dbh, int (*walk_func)(char *key, void *data, int len))
{
	DBT key, data;
	DBC *dbc;
	int rc = 0;
	GET_DB(dbh);

	if (!walk_func)
		return -EINVAL;

	if ((rc = db->cursor(db, NULL, &dbc, 0))) {
		db->close(db, 0);
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

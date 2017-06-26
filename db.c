#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <db.h>

#include "samlib.h"

/* DB_VERSION_MAJOR is not defined in FreeBSD. */


static DB *global_db;

static inline void dbclose(DB *db)
{
	if (db) {
#ifdef DB_VERSION_MAJOR
		db->close(db, 0);
#else
		db->close(db);
#endif
	}
}

static void db_close_global(void)
{
	if (global_db) {
		dbclose(global_db);
		global_db = NULL;
	}
}

int db_open(const char *dbname, uint32_t flags, void **dbh)
{
	DB *db;

	if (!dbh && global_db)
		return -EBUSY;

#ifdef DB_VERSION_MAJOR
	int rc = db_create(&db, NULL, 0);
	if (rc)
		return rc;

	rc = db->open(db, NULL, dbname, NULL, DB_BTREE, flags, 0664);
	if (rc)
		return rc;
#else
	db = dbopen(dbname, flags, 0664, DB_BTREE, NULL);
	if (!db)
		return errno;
#endif

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
		dbclose(db);
	return 0;
}

int db_put_raw(void *dbh, const void *key, int klen, void *val, int len, unsigned flags)
{
	DBT key_dbt, data_dbt;
	GET_DB(dbh);

	memset(&key_dbt, 0, sizeof(key_dbt));
	memset(&data_dbt, 0, sizeof(data_dbt));

	key_dbt.data = (void *)key;
	key_dbt.size = klen;
	if (len) {
		data_dbt.data = val;
		data_dbt.size = len;
	}

#ifdef DB_VERSION_MAJOR
	return db->put(db, NULL, &key_dbt, &data_dbt, flags);
#else
	return db->put(db, &key_dbt, &data_dbt, flags);
#endif
}

/* Returns 0 on success */
int db_put(void *dbh, const char *keystr, void *val, int len, unsigned flags)
{
	return db_put_raw(dbh, keystr, strlen(keystr) + 1, val, len, flags);
}

int db_put_str(void *dbh, const char *keystr, const char *valstr)
{
	if (valstr)
		return db_put(dbh, (char *)keystr, (char *)valstr, strlen(valstr) + 1, 0);
	else
		return db_put(dbh, keystr, NULL, 0, 0);
}

/* Handy function for keeping track of counts or sizes. Reads the old
 * value and adds in the new value.
 */
int db_update_long(void *dbh, const char *keystr, long update)
{
	long cur;

	if (db_get(dbh, keystr, &cur, sizeof(cur)) > 0)
		update += cur;

	return db_put(dbh, keystr, &update, sizeof(update), 0);
}

/* Returns val len on success */
int db_get_raw(void *dbh, const void *key, int klen, void *val, int len)
{
	int rc;
	DBT key_dbt, data_dbt;
	GET_DB(dbh);

	memset(&key_dbt, 0, sizeof(key_dbt));
	memset(&data_dbt, 0, sizeof(data_dbt));

	key_dbt.data = (void *)key;
	key_dbt.size = klen;

#ifdef DB_VERSION_MAJOR
	rc = db->get(db, NULL, &key_dbt, &data_dbt, 0);
#else
	rc = db->get(db, &key_dbt, &data_dbt, 0);
#endif
	if (rc)
		return rc < 0 ? rc : -rc;

	if (len > data_dbt.size)
		len = data_dbt.size;
	memcpy(val, data_dbt.data, len);

	return len;
}

int db_get(void *dbh, const char *keystr, void *val, int len)
{
	return db_get_raw(dbh, keystr, strlen(keystr) + 1, val, len);
}

/* The difference between db_get() and db_get_str() is that
 * db_get_str() guarantees a null terminated string if the return > 0.
 */
int db_get_str(void *dbh, const char *keystr, char *valstr, int len)
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

int db_peek(void *dbh, const char *keystr)
{
	DBT key;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));

	key.data = (char *)keystr;
	key.size = strlen(keystr) + 1;

#ifdef DB_VERSION_MAJOR
	return db->get(db, NULL, &key, NULL, 0);
#else
	return db->get(db, &key, NULL, 0);
#endif
}

int db_del(void *dbh, const char *keystr)
{
	DBT key;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));

	key.data = (char *)keystr;
	key.size = strlen(key.data) + 1;

#ifdef DB_VERSION_MAJOR
	return db->del(db, NULL, &key, 0);
#else
	return db->del(db, &key, 0);
#endif
}

/* WARNING: Assumes data is a string! */
int db_walk_puts(const char *key, void *data, int len)
{
	printf("%s: %.*s\n", key, len, (char *)data);
	return 0;
}

int db_walk_long(const char *key, void *data, int len)
{
	printf("%s: %ld\n", key, *(long *)data);
	return 0;
}

int db_walk(void *dbh, int (*walk_func)(const char *key, void *data, int len))
{
#ifdef DB_VERSION_MAJOR
	DBT key, data;
	DBC *dbc;
	int rc = 0;
	GET_DB(dbh);

	if (!walk_func)
		return -EINVAL;

	if ((rc = db->cursor(db, NULL, &dbc, 0))) {
		dbclose(db);
		return rc;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	while (dbc->c_get(dbc, &key, &data, DB_NEXT) == 0)
		if ((rc = walk_func(key.data, data.data, data.size)))
			break;

	dbc->c_close(dbc);

	return rc;
#else
	DBT key, data;
	int rc = 0, flag = R_FIRST;
	GET_DB(dbh);

	if (!walk_func)
		return -EINVAL;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	while (db->seq(db, &key, &data, flag) == 0) {
		if ((rc = walk_func(key.data, data.data, data.size)))
			break;
		flag = R_NEXT;
	}

	return rc;
#endif
}

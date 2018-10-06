#include <stdlib.h>
/* Hardcoded to make sure we get the right file */
#undef __DBINTERFACE_PRIVATE
#include "db.1.85/include/db.h"
#include "samlib.h"

static DB *global_db;

#ifdef WIN32
#undef close
#endif

static inline void dbclose(DB *db)
{
	if (db)
		db->close(db);
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

	db = dbopen(dbname, flags, 0664, DB_BTREE, NULL);
	if (!db)
		return errno;

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

int db_put_raw(void *dbh, const void *key, int klen, const void *val, int len, unsigned flags)
{
	DBT key_dbt, data_dbt;
	GET_DB(dbh);

	memset(&key_dbt, 0, sizeof(key_dbt));
	memset(&data_dbt, 0, sizeof(data_dbt));

	key_dbt.data = (void *)key;
	key_dbt.size = klen;
	if (len) {
		data_dbt.data = (void *)val;
		data_dbt.size = len;
	}

	return db->put(db, &key_dbt, &data_dbt, flags);
}

/* Returns 0 on success */
int db_put(void *dbh, const char *keystr, const void *val, int len)
{
	return db_put_raw(dbh, keystr, strlen(keystr) + 1, val, len, 0);
}

int db_put_str(void *dbh, const char *keystr, const char *valstr)
{
	if (valstr)
		return db_put_raw(dbh, keystr, strlen(keystr) + 1, valstr, strlen(valstr) + 1, 0);
	else
		return db_put_raw(dbh, keystr, strlen(keystr) + 1, NULL, 0, 0);
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

	data_dbt.data = (void *)val;
	data_dbt.size = len;

	rc = db->get(db, &key_dbt, &data_dbt, 0);
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

	return db->get(db, &key, NULL, 0);
}

int db_del(void *dbh, const char *keystr)
{
	DBT key;
	GET_DB(dbh);

	memset(&key, 0, sizeof(key));

	key.data = (char *)keystr;
	key.size = strlen(key.data) + 1;

	return db->del(db, &key, 0);
}

int db_walk(void *dbh, int (*walk_func)(const char *key, void *data, int len))
{
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
}

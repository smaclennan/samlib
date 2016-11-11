#ifndef __SAMLIB_H__
#define __SAMLIB_H__

#define SAMLIB_MAJOR 1
#define SAMLIB_MINOR 0

#include <stdint.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define ONE_MINUTE		60
#define ONE_HOUR		(ONE_MINUTE * 60)
#define ONE_DAY			(ONE_HOUR * 24)

/* Returns duration in seconds. Suffix: dhms. Supports fractional values.
 * Warning: exits on error.
 */
unsigned long get_duration(const char *str);

char *nice_duration(unsigned long duration, char *str, int len);

/* Returns memory length in bytes. Suffix: gmkb.
 * Warning: exits on error.
 */
unsigned long get_mem_len(const char *str);

unsigned nice_mem_len(unsigned long size, char *ch);

/* Read a file line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * Returns 0 on success, -1 on file or memory error, or 1 if
 * line_func() returns non-zero.
 */
int readfile(int (*line_func)(char *line, void *data), void *data,
			 const char *path, unsigned flags);

#define RF_IGNORE_EMPTY			1
#define RF_IGNORE_COMMENTS		2

/* Read a pipe command a line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * The command must be less than 1k.
 *
 * Returns pclose exit status on success, -1 on file or memory
 * error. If the line_func() returns non-zero, the return is -1 and
 * the errno is set to EINTR.
 */
int readcmd(int (*line_func)(char *line, void *data), void *data, const char *fmt, ...);

/* Copy a file.
 * Returns number of bytes copied or < 0 on error:
 *    -1 = I/O error
 *    -2 = error on from
 *    -3 = error on to
 */
long copy_file(const char *from, const char *to);

/* This mimics the shell's `mkdir -p' */
int mkdir_p(const char *dir);

int do_system(const char *fmt, ...);

void samlib_version(int *major, int *minor);
const char *samlib_versionstr(void);

/* The traditional binary dump with hex on left and chars on right. */
void binary_dump(const uint8_t *buf, int len);

/* If set, then get_month() assumes months are 1 based. Default is 0
 * based.
 */
extern int months_one_based;
const char *get_month(unsigned month, int long_months);

/* Can handle a difference of about 66 hours on 32 bit systems and
 * about 150 years on 64 bit systems. Optimized for delta < 1 second.
 */
ulong delta_timeval(const struct timeval *start, const struct timeval *end);

int timeval_delta_valid(const struct timeval *start,
						const struct timeval *end);

/* Returns 0 if a valid delta was calculated, an errno if not.
 * Note that delta can point to start or end.
 */
int timeval_delta(const struct timeval *start,
				  const struct timeval *end,
				  struct timeval *delta);

/* Same as timeval_delta but returns a double. Returns INFINITY on errror. */
double timeval_delta_d(const struct timeval *start,
					   const struct timeval *end);

/* Same as timeval_delta() but doesn't care if t1 before t2. */
int timeval_delta2(const struct timeval *t1,
				   const struct timeval *t2,
				   struct timeval *delta);

/* The opaque walkfile structure. If you pass NULL to the walkfile
 * functions they will use a builtin global. For multi-threaded apps
 * you must supply a walkfile struct.
 */
struct walkfile_struct {
	int (*file_func)(const char *path, struct stat *sbuf);
	void *ignores;
	void *filters;
	int flags;
};

/* Main function. Path can be a directory or a file. The file_func()
 * will be called on every file except . (dot) files. Honors both
 * ignores and filter if set.
 *
 * Note: By default walkfiles only walks regular files. The flags arg
 * can contain any of the stat(2) file types to walk them too.
 *
 * Note 2: Because S_IFCHR is a subset of S_IFBLK, S_IFBLK will also
 * match S_IFCHR. Check the real mode in your file function.
 */
int walkfiles(struct walkfile_struct *walk, const char *path,
			  int (*file_func)(const char *path, struct stat *sbuf),
			  int flags);

/* For backwards compatibility */
#define WALK_LINKS S_IFLNK

/* Print out what walkfiles is doing for debugging. */
#define WALK_VERBOSE 1

/* Don't walk directories on other filesystems (like find -xdev) */
#define WALK_XDEV 2

/* Adds a regular expression of paths to ignore. */
void add_ignore(struct walkfile_struct *walk, const char *str);

/* Check if a path matches one of the ignores. */
int check_ignores(struct walkfile_struct *walk, const char *path);

/* Sets a filter to match on every file. This uses file globbing, not
 * regular expressions.
 */
void add_filter(struct walkfile_struct *walk, const char *pat);

/* Check if the file name matches a filter */
int check_filters(struct walkfile_struct *walk, const char *fname);

/* Example of ignores vs filtering. If you only want to process text
 * files, you might use a filter of "*.txt". If you want to ignore all
 * text files, you could use an ignore of "\.txt$".
 */

/* md5 functions */

typedef struct md5_ctx {
	uint32_t abcd[4];
	uint8_t buf[64];
	int cur;
	uint32_t size;
} md5ctx;

#define MD5_DIGEST_LEN 16

void md5(const void *data, int len, uint8_t *hash);
void md5_init(md5ctx *ctx);
void md5_update(md5ctx *ctx, const void *data, int len);
void md5_final(md5ctx *ctx, uint8_t *hash);
char *md5str(uint8_t *hash, char *str);

/* IP functions */

/* Returns 0 on success. The args addr and/or mask can be NULL. */
int ip_addr(const char *ifname, struct in_addr *addr, struct in_addr *mask);

/* Returns 0 on success or an errno suitable for gai_strerror().
 * The ipv4 arg should be at least INET_ADDRSTRLEN long or NULL.
 * The ipv6 arg should be at least INET6_ADDRSTRLEN long or NULL.
 */
int get_address(const char *hostname, char *ipv4, char *ipv6);

/* ipv4 only... but useful. Returns in host order. */
uint32_t get_address4(const char *hostname);

/* DB functions - requires -ldb
 *
 * These functions implement a simple interface to Berkley DB btree
 * implementation. The dbh is defined as void so that users of other
 * samlib functions don't have to include db.h (which may not be
 * installed). If dbh is NULL, a global DB handle is used.
 */

/* The flags are for the DB->open() function. */
int db_open(char *dbname, uint32_t flags, void **dbh);
int db_close(void *dbh);
int db_put(void *dbh, char *keystr, void *val, int len, unsigned flags);
int db_put_str(void *dbh, char *keystr, char *valstr);
int db_update_long(void *dbh, char *keystr, long update);
#define db_inc_long(d, k) db_update_long((d), (k), 1)
#define db_dec_long(d, k) db_update_long((d), (k), -1)
int db_get(void *dbh, char *keystr, void *val, int len);
int db_get_str(void *dbh, char *keystr, char *valstr, int len);
int db_peek(void *dbh, char *keystr);
int db_del(void *dbh, char *keystr);
int db_walk(void *dbh, int (*walk_func)(char *key, void *data, int len));

/* Advantages of xorshift128plus() over random().
 *     1. It is faster. (~7x)
 *     2. It returns a full 64 bits.
 */
uint64_t xorshift128plus(void);
uint64_t xorshift128plus_r(uint64_t *seed); /* thread safe version */

/* The seed arg is where to put the seed for xorshift128plus_r(). For
 * xorshift128plus() just pass NULL.
 */
void xorshift_seed(uint64_t *seed);

/* If must_helper is set, and any of the must_* functions fail, then
 * must_helper will be called with the command that failed and the
 * size.
 *
 * If must_helper is not set, then the if the must_* functions fail
 * they will print "Out of memory" and exit(1).
 */
extern void (*must_helper)(const char *what, int size);

char *must_strdup(const char *s);
void *must_alloc(size_t size);
void *must_calloc(int nmemb, int size);
void *must_realloc(void *ptr, int size);

#endif

#ifndef __SAMLIB_H__
#define __SAMLIB_H__

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef WIN32
#include "win32/samwin32.h"
#else
#include <unistd.h>
#include <netinet/in.h>
#endif

#define SAMLIB_MAJOR 1
#define SAMLIB_MINOR 2

extern const char *samlib_version;

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

unsigned nice_mem_len(uint64_t size, char *ch);

/* Adds commas. Not thread safe */
char *nice_number(long number);

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

/* Create a file of set length and mode. If mode is 0, a reasonable default is chosen. */
int create_file(const char *fname, off_t length, mode_t mode);

/* This mimics the shell's `mkdir -p' */
int mkdir_p(const char *dir);

int do_system(const char *fmt, ...);

/* The traditional binary dump with hex on left and chars on right. */
void binary_dump(const uint8_t *buf, int len);

/* Zero based */
extern const char *short_month[];
extern const char *long_month[];

/* If set, then get_month() assumes months are 1 based. Default is 0
 * based.
 */
extern int months_one_based;
const char *get_month(unsigned month, int long_months);

/* Can handle a difference of about 66 hours on 32 bit systems and
 * about 150 years on 64 bit systems. Optimized for delta < 1 second.
 */
unsigned long delta_timeval(const struct timeval *start,
							const struct timeval *end);

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

#define MD5_DIGEST_LEN 16

typedef struct md5_ctx {
	uint32_t abcd[MD5_DIGEST_LEN / sizeof(uint32_t)];
	uint8_t buf[64];
	int cur;
	uint32_t size;
} md5ctx;

void md5(const void *data, int len, uint8_t *hash);
void md5_init(md5ctx *ctx);
void md5_update(md5ctx *ctx, const void *data, int len);
void md5_final(md5ctx *ctx, uint8_t *hash);
char *md5str(uint8_t *hash, char *str);

/* sha256 functions */

#define SHA256_DIGEST_SIZE (256 / 8)

typedef struct sha256ctx {
	uint8_t block[SHA256_DIGEST_SIZE * 2];
	uint32_t h[SHA256_DIGEST_SIZE / sizeof(uint32_t)]; /* Message Digest */
	uint64_t len;						/* Message length in bytes */
	int_least16_t index;				/* Message_Block array index */
										/* 512-bit message blocks */
} sha256ctx;

int sha256(const void *data, int len, uint8_t *digest);
int sha256_init(sha256ctx *ctx);
int sha256_update(sha256ctx *ctx, const uint8_t *bytes, unsigned bytecount);
int sha256_final(sha256ctx *ctx, uint8_t *digest);
char *sha256str(uint8_t *digest, char *str);

/* base64 functions */

/* If this is set to non-zero than the url safe alphabet is used for
 * encoding. For decoding both are supported.
 * Not thread safe unless all threads use the same value.
 */
extern int base64url_safe;

int base64_encode(char *dst, int dlen, const uint8_t *src, int len);
int base64_encoded_len(int len);
int base64_decode(uint8_t *dst, int dlen, const char *src, int len);
int base64_decoded_len(int len);

/* Mainly for IP header checksums */
uint16_t chksum16(const void *buf, int count);

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
int db_open(const char *dbname, uint32_t flags, void **dbh);
int db_close(void *dbh);
int db_put(void *dbh, const char *keystr, void *val, int len, unsigned flags);
int db_put_str(void *dbh, const char *keystr, const char *valstr);
int db_put_raw(void *dbh, const void *key, int klen, void *val, int len, unsigned flags);
int db_update_long(void *dbh, const char *keystr, long update);
#define db_inc_long(d, k) db_update_long((d), (k), 1)
#define db_dec_long(d, k) db_update_long((d), (k), -1)
int db_get(void *dbh, const char *keystr, void *val, int len);
int db_get_str(void *dbh, const char *keystr, char *valstr, int len);
int db_get_raw(void *dbh, const void *key, int klen, void *val, int len);
int db_peek(void *dbh, const char *keystr);
int db_del(void *dbh, const char *keystr);
int db_walk(void *dbh, int (*walk_func)(const char *key, void *data, int len));
/* Sample db_walk function.
 * WARNING: Assumes data is a string!
 */
int db_walk_puts(const char *key, void *data, int len);
int db_walk_long(const char *key, void *data, int len);

#ifndef DB_CREATE
#define DB_CREATE (O_CREAT | O_RDWR)
#endif

/* Advantages of xorshift128plus() over random().
 *     1. It is faster. (~7x)
 *     2. It returns a full 64 bits.
 */
typedef struct { uint64_t seed[2]; } xorshift_seed_t;
uint64_t xorshift128plus(void);
uint64_t xorshift128plus_r(xorshift_seed_t *seed); /* thread safe version */

/* The seed arg is where to put the seed for xorshift128plus_r(). For
 * xorshift128plus() just pass NULL.
 *
 * Note: xorshift128plus() will self-seed, but will exit if seeding fails.
 */
int xorshift_seed(xorshift_seed_t *seed);
void must_xorshift_seed(xorshift_seed_t *seed);

/* Somebody didn't like the names... so nicer names. */
static inline void sam_srand(void) { must_xorshift_seed(NULL); }
static inline uint64_t sam_rand(void) { return xorshift128plus(); }

/* If must_helper is set, and any of the must_* functions fail, then
 * must_helper will be called with the command that failed and the
 * size.
 *
 * If must_helper is not set and the must_* functions fail they will
 * print "Out of memory" and exit(1).
 */
extern void (*must_helper)(const char *what, int size);

char *must_strdup(const char *s);
void *must_alloc(size_t size);
void *must_calloc(int nmemb, int size);
void *must_realloc(void *ptr, int size);

/* Note: /proc/pid/cmdline is currently limited to 4k */
int readproccmdline(pid_t pid, char *buf, int len);
int readproccmd(pid_t pid, char *buf, int len);

/* First few fields of /proc/<pid>/stat.
 * state can be "RSDZTW" where:
 * R is running,
 * S is sleeping in an interruptible wait,
 * D is waiting in uninterruptible disk sleep
 * Z is zombie
 * T is traced or stopped (on a signal)
 * W is paging.
 */
struct procstat_min {
	pid_t pid;
	char comm[18]; /* executable name with brackets */
	char state;
	pid_t ppid;
	pid_t pgrp;
	pid_t session;
};

int readprocstat(pid_t pid, struct procstat_min *stat);


/* For _findpid the matched command will be in buf so that you can
 * further refine the match.
 * Both _findpid and findpid are thread safe.
 */
pid_t _findpid(const char *cmd, pid_t start_pid, char *buf, int len);
pid_t findpid(const char *cmd, pid_t start_pid);

/* Requires -rdynamic to be useful */
void dump_stack(void);

#ifdef WIN32
/* Windows still cannot handle void * correctly */
#define WIN32_COERCE (unsigned char *)
#else
#define WIN32_COERCE
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif

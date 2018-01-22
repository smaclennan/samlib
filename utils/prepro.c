#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

/* TODO
 *
 * - should check name and define limits
 * - doesn't support #define with complex values
 *
 * Limitations
 *
 * - assumes code compiles! No attempt to handle malformed lines
 */

#define DBG(fmt, ...)

#define IN_PREPRO
#include "strip-comments2.c"

int verbose;

int file_nest = 0;

#define MAX_NESTED 4

struct prepro {
	int inif;
	int skipping[MAX_NESTED];
	int set_true[MAX_NESTED];
};

static struct define {
	char *name;
	int val;
} *defines;
int n_defines;

struct inc {
	const char *fname;
	struct inc *next;
} *incs;

static int add_inc(const char *fname)
{
	struct inc *inc;

	for (inc = incs; inc; inc = inc->next)
		if (strcmp(fname, inc->fname) == 0)
			return 0;

	inc = calloc(1, sizeof(struct inc));
	fname = strdup(fname);
	if (!inc || !fname) {
		puts("Out of memory!");
		exit(1);
	}

	inc->fname = fname;
	inc->next = incs;
	incs = inc;
	return 1;
}

static int check_defined(const char *define)
{
	int i;
	for (i = 0; i < n_defines; ++i)
		if (strcmp(define, defines[i].name) == 0)
			return 1;

	return 0;
}

static int lookup_define(const char *define)
{
	int i;
	for (i = 0; i < n_defines; ++i)
		if (strcmp(define, defines[i].name) == 0)
			return defines[i].val;

	return 0;
}

static void add_define(const char *define, int val)
{
	static int max_defines;

	if (defines == NULL) {
		max_defines = 16;
		if (!(defines = malloc(max_defines * sizeof(struct define)))) {
			puts("Out of memory!");
			exit(1);
		}

		/* standard defines */
		add_define("__unix__", 1);
		add_define("__linux__", 1);
		add_define("__GNUC__", 1);
	}

	if (check_defined(define))
		return;

	if (n_defines == max_defines) {
		max_defines += 4;
		if (!(defines = realloc(defines, max_defines * sizeof(struct define)))) {
			puts("Out of memory!");
			exit(1);
		}
	}

	if (!(defines[n_defines].name = strdup(define))) {
		puts("Out of memory!");
		exit(1);
	}
	defines[n_defines].val = val;
	++n_defines;
}

static void undefine(const char *define)
{
	int i;

	for (i = 0; i < n_defines; ++i)
		if (strcmp(defines[i].name, define) == 0) {
			memmove(&defines[i], &defines[i + 1], (n_defines - i - 1) * sizeof(struct define));
			--n_defines;
		}
}

static void zedit_defines(void)
{
	add_define("R_OK", R_OK);
	add_define("SIGHUP", SIGHUP);
	add_define("SIGTERM", SIGTERM);
	add_define("SIGWINCH", SIGWINCH);
#ifdef WNOWAIT
	add_define("WNOWAIT", WNOWAIT);
#endif
	add_define("HAVE_CONFIG_H", 1);
	add_define("TAB3", 1);
}

static void process_file(const char *fname, FILE *out);

/**************************************************************
 *
 * #if processing code. The upper levels have already dealt with
 * nesting and stripped comments. They also have skipped the keyword.
 *
 * WARNING: Does not handle errors gracefully!
 */

/* ifdef and ifndef */
int simple_if(char *line)
{
	char *s;

	while (isspace(*line)) ++line;
	for (s = line; isalnum(*line) || *line == '_'; ++line) ;
	*line = 0;
	return check_defined(s);
}

/* reasonable default */
#define MAX_OPS 10

struct values {
	char op[3];
	int val;
};

static struct values fvals[] = {
	{ "N",  8 }, /* number */

	{ "(",  0 }, { ")",  8 },

	{ "==", 6 }, { "!=", 6 },
	{ ">=", 6 }, { ">",  6 },
	{ "<=", 6 }, { "<",  6 },

	{ "||", 4 }, { "&&", 4 },

	{ "", 0 } /* terminator */
};

static struct values gvals[] = {
	{ "N",  7 }, /* number */

	{ "(",  7 }, { ")",  0 },

	{ "==", 5 }, { "!=", 5 },
	{ ">",  5 }, { ">=", 5 },
	{ "<",  5 }, { "<=", 5 },

	{ "||", 3 }, { "&&", 3 },

	{ "", 0 } /* terminator */
};

#define NUMBER 0
#define TERMINATOR 11

static int is_op(int op)
{
	return op > 2 && op < TERMINATOR;
}

static int ops[MAX_OPS];
static int cur_op;

static int nums[MAX_OPS];
static int cur_num;

static int lookup(const struct values *v, char **op, int *opn)
{
	int i;

	while (isspace(**op)) ++(*op);

	for (i = 0; *v->op; ++v, ++i)
		if (strncmp(*op, v->op, strlen(v->op)) == 0) {
			*op += strlen(v->op);
			*opn = i;
			return v->val;
		}

	if (!**op) {
		*opn = TERMINATOR;
		return 0;
	}

	assert(0); /* invalid op */
}

static void push_op(int op)
{
	assert(cur_op < MAX_OPS);
	ops[cur_op++] = op;
}

static int pop_op(void)
{
	assert(cur_op > 0);
	return ops[--cur_op];
}

static int top_op(void)
{
	if (cur_op == 0)
		return TERMINATOR;
	return ops[cur_op - 1];
}

static void push_num(int num)
{
	assert(cur_num < MAX_OPS);
	nums[cur_num++] = num;
}

static int pop_num(void)
{
	assert(cur_num > 0);
	return nums[--cur_num];
}

static int calc_f(int op)
{
	return fvals[op].val;
}

static inline void push_num_not(int num, int not)
{
	if (not)
		num = !num;
	push_num(num);
}

/** Precedence function `g'.
 * Only this function is ever looking at a define.
 * It reads the number, pushes it on the nums stack,
 */
static int calc_g_num(char **p, int *op)
{
	char save, *s = *p;
	int not = 0;

	if (**p == '!') {
		not = 1;
		++(*p);
		s = *p;
	}

	if (isalpha(**p)) {
		if (strncmp(*p, "defined(", 8) == 0) {
			*p += 8;
			s = *p;
			*p = strchr(*p, ')');
			assert(*p);
			save = **p; /* for debugging */
			**p = 0;
			push_num_not(check_defined(s), not);
			**p = save;
			++(*p);
		} else {
			while (isalnum(**p) || **p == '_') ++(*p);
			save = **p;
			**p = 0;
			push_num_not(lookup_define(s), not);
			**p = save;
		}
		*op = NUMBER;
		return gvals[NUMBER].val;
	} else if (isdigit(**p)) {
		push_num_not(strtol(*p, p, 0), not);
		*op = NUMBER;
		return gvals[NUMBER].val;
	}

	return lookup(gvals, p, op);
}

static int calc_g(int op)
{
	return gvals[op].val;
}

#define OP(op) push_num(one op two)

/* if and elif */
int complex_if(char *line)
{
	int f_val, g_val, op, shift = 1;

	while (*line || top_op() != TERMINATOR) {
		while (isspace(*line)) ++line;

		f_val = calc_f(top_op());
		if (shift)
			g_val = calc_g_num(&line, &op);
		else
			g_val = calc_g(op);
		DBG("f_val %d (%s) g_val %d (%s) %s\n",
			f_val, fvals[top_op()].op, g_val, fvals[op].op,
			f_val <= g_val ? "shift" : "reduce");

		if (f_val <= g_val) {
			/* shift */
			push_op(op);
			shift = 1;
		} else {
			/* reduce */
			shift = 0;
			do {
				int op = pop_op();
				if (is_op(op)) {
					int two = pop_num();
					int one = pop_num();

					switch (op) {
					case 3:
						OP(==);
						break;
					case 4:
						OP(!=);
						break;
					case 5:
						OP(>=);
						break;
					case 6:
						OP(>);
						break;
					case 7:
						OP(<=);
						break;
					case 8:
						OP(<);
						break;
					case 9:
						OP(||);
						break;
					case 10:
						OP(&&);
						break;
					}
				}

				f_val = calc_f(top_op());
				g_val = calc_g(op);
				DBG("f_val %d g_val %d\n", f_val, g_val);
			} while (f_val >= g_val);
		}
	}

	return pop_num();
}

/*
 * #if processing code.
 *
 **************************************************************/

static void handle_if(struct prepro *pp, char *p)
{
	if (pp->skipping[pp->inif]) {
		/* if skipping at higher level, skipping at this level */
		++pp->inif;
		assert(pp->inif < MAX_NESTED);
		pp->skipping[pp->inif] = 1;
		pp->set_true[pp->inif] = 1;
		return;
	}

	int result;

	while (isspace(*p)) ++p;

	if (strncmp(p, "#ifdef", 6) == 0)
		result = simple_if(p + 6);
	else if (strncmp(p, "#ifndef", 7) == 0)
		result = !simple_if(p + 7);
	else /* #if */
		result = complex_if(p + 3);

	++pp->inif;
	assert(pp->inif < MAX_NESTED);
	pp->skipping[pp->inif] = !result;
	pp->set_true[pp->inif] = result;
}

static void handle_elseif(struct prepro *pp, char *line)
{
	if (pp->set_true[pp->inif] == 1)
		pp->skipping[pp->inif] = 1;
	else {
		while (isspace(*line)) ++line;
		line += 5; /* #elif */
		while (isspace(*line)) ++line;
		int result = complex_if(line);
		pp->skipping[pp->inif] = !result;
		pp->set_true[pp->inif] = result;
	}
}

static void handle_else(struct prepro *pp, const char *line)
{
	if (pp->inif > 0 && pp->skipping[pp->inif - 1] == 1)
		return;
	pp->skipping[pp->inif] = pp->set_true[pp->inif];
}

static void handle_endif(struct prepro *pp, const char *line)
{
	pp->skipping[pp->inif] = 0; /* reset */
	pp->set_true[pp->inif] = 0; /* reset */
	--pp->inif;
	assert(pp->inif >= 0);
}

static void handle_include(struct prepro *pp, const char *line, FILE *out)
{
	const char *p = line;
	char name[100], *s;

	while (isspace(*p)) ++p;
	p += 8; /* #include */
	while (isspace(*p)) ++p;
	if (*p == '<') {
		fputs(line, out);
		return;
	}

	assert(*p++ == '"');
	for (s = name; *p && *p != '"'; ++p)
		*s++ = *p;
	*s = 0;

	if (add_inc(name))
		process_file(name, out);
}

static void handle_define(struct prepro *pp, const char *line, FILE *out)
{
	const char *p = line;
	char name[100], *s, *e;
	int val = 1;

	fputs(line, out);

	while (isspace(*p)) ++p;
	p += 7; /* #define */
	while (isspace(*p)) ++p;

	for (s = name; isalnum(*p) || *p == '_'; ++p)
		*s++ = *p;
	*s = 0;
	if (*p == '(') {
		/* macro - just mark it as defined */
		add_define(name, 1);
		return;
	}

	val = strtol(p, &e, 0);
	if (p == e)
		val = 1; /* not a value define */

	add_define(name, val);
}

/* str must be a constant string */
#define MATCH(str) (strncmp(p, str, sizeof(str) - 1) == 0)

static void process_one(int in, FILE *out)
{
	char line[1024], *p;
	struct prepro prepro;
	struct strip_ctx ctx;

	memset(&prepro, 0, sizeof(prepro));
	strip_ctx_init(&ctx, in);

	while (mygets(&ctx, line, sizeof(line))) {
		for (p = line; isspace(*p); ++p) ;
		if (MATCH("#if"))
			handle_if(&prepro, p);
		else if (MATCH("#elif"))
			handle_elseif(&prepro, p);
		else if (MATCH("#else"))
			handle_else(&prepro, p);
		else if (MATCH("#endif"))
			handle_endif(&prepro, p);
		else if (!prepro.skipping[prepro.inif]) {
			if (MATCH("#include"))
				handle_include(&prepro, p, out);
			else if (MATCH("#define"))
				handle_define(&prepro, p, out);
			else if (verbose)
				fprintf(out, "%d: %s", prepro.inif, line);
			else
				fputs(line, out);
		} else if (verbose > 1)
			fprintf(out, "%dS: %s", prepro.inif, line);
	}
}

static void process_file(const char *fname, FILE *out)
{
	++file_nest;

	int fd = open(fname, O_RDONLY);
	if (fd >= 0) {
		process_one(fd, out);
		close(fd);
	} else
		perror(fname);

	--file_nest;
}

int main(int argc, char *argv[])
{
	int c;
	FILE *out = stdout;
	const char *oname = NULL;

	while ((c = getopt(argc, argv, "D:U:o:vz")) != EOF)
		switch (c) {
		case 'D':
			add_define(optarg, 1);
			break;
		case 'U':
			undefine(optarg);
			break;
		case 'o':
			oname = optarg;
			break;
		case 'v':
			++verbose;
			break;
		case 'z':
			zedit_defines();
			break;
		default:
			puts("Sorry!");
			exit(1);
		}

	if (oname) {
		if (!(out = fopen(oname, "w"))) {
			perror(oname);
			exit(1);
		}
	}

	if (argc == 1)
		process_one(fileno(stdin), out);
	else
		for (c = optind; c < argc; ++c)
			process_file(argv[c], out);

	if (oname)
		fclose(out);

	return 0;
}

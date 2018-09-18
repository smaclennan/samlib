#include <stdarg.h>
#include <ctype.h>
#include "samlib.h"

#define WIDTH_MASK		0x0fffff
#define SAW_NEG			0x100000
#define SAW_ZERO		0x200000
#define SAW_LONG		0x400000

struct outbuff {
	char *str;
	int len;
	int n;
};

static void reverse_string(char *start, char *end)
{
	int len = (end - start) / 2;
	--end;
	for (int i = 0; i < len; ++i) {
		char temp = *end;
		*end = *start;
		*start = temp;
		start++;
		end--;
	}
}

/** Integer to ascii.
 * @param val The integer to convert.
 * @param out The output string.
 * @return A pointer to out.
 */
char *int2str(long val, char *out)
{
	if (val == 0)
		strcpy(out, "0");
	else {
		char *p = out;
		int neg = val < 0;

		if (neg)
			val = -val;

		while (val > 0) {
			*p++ = (val % 10) + '0';
			val /= 10;
		}
		if (neg) *p++ = '-';
		*p = 0;

		reverse_string(out, p);
	}

	return out;
}

/** Unsigned integer to ascii.
 * @param val The integer to convert.
 * @param out The output string.
 * @return A pointer to out.
 */
char *uint2str(unsigned long val, char *out)
{
	if (val == 0)
		strcpy(out, "0");
	else {
		char *p = out;

		while (val > 0) {
			*p++ = (val % 10) + '0';
			val /= 10;
		}
		*p = 0;

		reverse_string(out, p);
	}
	return out;
}

/** Hex to ascii.
 * @param val The integer to convert.
 * @param out The output string.
 * @return A pointer to out.
 */
char *hex2str(unsigned long val, char *out)
{
	if (val == 0)
		strcpy(out, "0");
	else {
		char *p = out;

		while (val > 0) {
			*p++ = tohex[val & 0xf];
			val >>= 4;
		}
		*p = 0;

		reverse_string(out, p);
	}
	return out;
}

static void outmemset(struct outbuff *out, char c, int size)
{
	if (size > 0) {
		out->n += size;
		size = MIN(size, out->len);
		if (size > 0) {
			memset(out->str, c, size);
			out->str += size;
			out->len -= size;
		}
	}
}

static void outmemcpy(struct outbuff *out, const char *str, int size)
{
	if (size > 0) {
		out->n += size;
		size = MIN(size, out->len);
		if (size > 0) {
			memcpy(out->str, str, size);
			out->str += size;
			out->len -= size;
		}
	}
}

static void outchar(struct outbuff *out, char ch)
{
	out->n++;
	if (out->len > 0) {
		*out->str++ = ch;
		--out->len;
	}
}

static void outstr(struct outbuff *out, const char *s, unsigned flags)
{
	int slen = strlen(s);
	int len = flags & WIDTH_MASK;
	int pad = len - slen;

	if ((flags & SAW_NEG) == 0) {
		outmemset(out, ' ', pad);
		pad = 0;
	}
	outmemcpy(out, s, slen);
	outmemset(out, ' ', pad);
}

static void outnum(struct outbuff *out, const char *s, unsigned flags)
{
	int slen = strlen(s);
	int len = flags & WIDTH_MASK;

	if (len)
		outmemset(out, (flags & SAW_ZERO) ? '0' : ' ', len - slen);

	outmemcpy(out, s, slen);
}

/** Lower level interface to strfmt() when you already have the va_list.
 * @param str The output string.
 * @param len The length of the output string.
 * @param fmt The output format.
 * @param ap The va_list.
 * @return The number of bytes in the source string (like snprintf).
 */
int strfmt_ap(char *str, int len, const char *fmt, va_list ap)
{
	struct outbuff out;
	char tmp[22];

	if (len < 1)
		return 0;

	out.str = str;
	out.len = len - 1; // leave room for NULL
	out.n = 0;

	while (*fmt) {
		if (*fmt == '%') {
			const char *save = fmt++;
			unsigned flags = 0, width = 0;

			if (*fmt == '-') {
				++fmt;
				flags |= SAW_NEG;
			}
			if (*fmt == '0')
				flags |= SAW_ZERO;
			while (isdigit(*fmt)) {
				width = width * 10 + *fmt - '0';
				++fmt;
			}
			flags |= width & WIDTH_MASK;
			if (*fmt == 'l') {
				++fmt;
				flags |= SAW_LONG;
			}
			switch (*fmt) {
			case 's':
				outstr(&out, va_arg(ap, char *), flags);
				break;
			case 'd':
				if (flags & SAW_LONG)
					int2str(va_arg(ap, long), tmp);
				else
					int2str(va_arg(ap, int), tmp);
				outnum(&out, tmp, flags);
				break;
			case 'u':
				if (flags & SAW_LONG)
					uint2str(va_arg(ap, unsigned long), tmp);
				else
					uint2str(va_arg(ap, unsigned), tmp);
				outnum(&out, tmp, flags);
				break;
			case 'x':
				if (flags & SAW_LONG)
					hex2str(va_arg(ap, unsigned long), tmp);
				else
					hex2str(va_arg(ap, unsigned long), tmp);
				outnum(&out, tmp, flags);
				break;
			default:
				outchar(&out, '%');
				fmt = save;
			}
		} else
			outchar(&out, *fmt);
		++fmt;
	}

	/* We leave room for the NULL */
	*out.str = 0;

	return out.n;
}

/** Poor man's snprintf.
 *
 * Supports a subset of printf: %s, %d, %u, %x, %l[dux]. Format can
 * contain a width and a minus (-) for left justify. Numbers starting
 * with 0 and a width are zero padded.
 *
 * @param str The output string.
 * @param len The length of the output string.
 * @param fmt The output format.
 * @param ... The zero or more arguments.
 * @return The number of bytes in the source (like snprintf).
 */
int strfmt(char *str, int len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = strfmt_ap(str, len, fmt, ap);
	va_end(ap);

	return n;
}

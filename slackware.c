#include "samlib.h"

/* Parse a Slackware package name into it's parts */
int pkgparse(const char *pkg_str, struct slack_pkg *pkg)
{
	/* isolate the basename */
	const char *base = strrchr(pkg_str, '/');
	if (base)
		++base;
	else
		base = pkg_str;

	pkg->name = strdup(base);
	if (!pkg->name) return ENOMEM;

	/* easiest to start at the end */
	char *p = strrchr(pkg->name, '-');
	if (!p) goto bad;
	pkg->build_tag = p + 1;
	*p = 0;

	p = strrchr(pkg->name, '-');
	if (!p) goto bad;
	pkg->arch = p + 1;
	*p = 0;

	p = strrchr(pkg->name, '-');
	if (!p) goto bad;
	pkg->version = p + 1;
	*p = 0;

	return 0;

bad:
	free(pkg->name);
	pkg->name = NULL;
	return EINVAL;
}

#include <sys/types.h>
#include <sys/sbuf.h>
#include <sys/uio.h>
#include <sys/extattr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <libutil.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "hbsdcontrol.h"

static int hbsdcontrol_verbose_flag;

const struct pax_feature_entry pax_features[] = {
	{
		"aslr",
		{
			[disable] = "hbsd.pax.noaslr",
			[enable]  = "hbsd.pax.aslr",
		},
	},
	{
		"segvguard",
		{
			[disable] = "hbsd.pax.nosegvguard",
			[enable]  = "hbsd.pax.segvguard",
		},
	},
	{
		"pageexec",
		{
			[disable] = "hbsd.pax.nopageexec",
			[enable]  = "hbsd.pax.pageexec",
		},
	},
	{
		"mprotect",
		{
			[disable] = "hbsd.pax.nomprotect",
			[enable]  = "hbsd.pax.mprotect",
		},
	},
	{
		"shlibrandom",
		{
			[disable] = "hbsd.pax.noshlibrandom",
			[enable]  = "hbsd.pax.shlibrandom",
		},
	},
	{
		"disallow_map32bit",
		{
			[disable] = "hbsd.pax.nodisallow_map32bit",
			[enable]  = "hbsd.pax.disallow_map32bit",
		},
	},
	{NULL, {0, 0}}
};

	
int
hbsdcontrol_set_extattr(const char *file, const char *feature, const int val)
{
	int	error;
	int	len;
	int	attrnamespace;
	struct sbuf *attrval = NULL;

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	attrval = sbuf_new_auto();
	sbuf_printf(attrval, "%d", val);
	sbuf_finish(attrval);

	len = extattr_set_file(file, attrnamespace, feature,
	    sbuf_data(attrval), sbuf_len(attrval));
	if (len >= 0 && hbsdcontrol_verbose_flag)
		warnx("%s: %s@%s = %s", file, "system", feature, sbuf_data(attrval));

	sbuf_delete(attrval);

	if (len == -1) {
		perror(__func__);
		errx(-1, "abort");
	}

	return (0);
}


int
hbsdcontrol_rm_extattr(const char *file, const char *feature)
{
	int error;
	int attrnamespace;

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	if (hbsdcontrol_verbose_flag)
		printf("reset feature: %s on file: %s\n", feature, file);

	error = extattr_delete_file(file, attrnamespace, feature);

	return (error);
}

int
hbsdcontrol_set_feature_state(const char *file, const char *feature, enum pax_feature_state state)
{
	int i;
	int error;

	error = 0;

	for (i = 0; pax_features[i].feature != NULL; i++) {
		if (strcmp(pax_features[i].feature, feature) == 0) {
			if (hbsdcontrol_verbose_flag) {
				printf("%s %s on %s\n",
				    state ? "enable" : "disable",
				    pax_features[i].feature, file);
			}

			error = hbsdcontrol_set_extattr(file, pax_features[i].entry[disable], !state);
			error |= hbsdcontrol_set_extattr(file, pax_features[i].entry[enable], state);

			break;
		}
	}

	return (error);
}


int
hbsdcontrol_rm_feature_state(const char *file, const char *feature)
{
	int i;
	int error;

	error = 0;

	for (i = 0; pax_features[i].feature != NULL; i++) {
		if (!strcmp(pax_features[i].feature, feature)) {
			if (hbsdcontrol_verbose_flag) 
				printf("reset %s on %s\n",
				    pax_features[i].feature, file);
			error = hbsdcontrol_rm_extattr(file, pax_features[i].entry[disable]);
			error |= hbsdcontrol_rm_extattr(file, pax_features[i].entry[enable]);

			break;
		}
	}

	return (error);
}

int
hbsdcontrol_set_verbose(const int level)
{

	hbsdcontrol_verbose_flag = level;

	return (hbsdcontrol_verbose_flag);
}

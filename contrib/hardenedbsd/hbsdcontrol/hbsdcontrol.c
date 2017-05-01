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
hbsdcontrol_list_extattrs(const char *file, char **features)
{
	int error;
	int attrnamespace;
	ssize_t nbytes;
	char *data;
	size_t pos;
	uint8_t len;

	nbytes = 1 * 1024 * 1024; // XXXOP
	data = NULL;
	pos = 0;

	if (features == NULL)
		err(-1, "%s", "features");

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	if (hbsdcontrol_verbose_flag)
		printf("list attrs on file: %s\n", file);

	nbytes = extattr_list_file(file, attrnamespace, NULL, nbytes);
	if (nbytes == -1) {
		error = EFAULT;
		goto out;
	}

	data = calloc(sizeof(char), nbytes);
	if (data == NULL) {
		error = ENOMEM;
		goto out;
	}

	nbytes = extattr_list_file(file, attrnamespace, data, nbytes);
	if (nbytes == -1) {
		error = EFAULT;
		goto out;
	}
	
	pos = 0;
	while (pos < nbytes) {
		int i;
		int j;
		size_t attr_len;

		/* see EXTATTR(2) about the data structure */
		len = data[pos++];

		/* Yes, this isn't an optimized algorithm. */
		for (i = 0; pax_features[i].feature != NULL; i++) {
			/* The 2 comes from enum pax_feature_state's size */
			for (j = 0; j < 2; j++) {
				attr_len = strlen(pax_features[i].entry[j]);
				if (attr_len != len) {
					/* Fast path, skip if the size of attribute differs. */
					continue;
				}

				if (!memcmp(pax_features[i].entry[j], &data[pos], attr_len)) {
					// entry = stdup(pax_features[i].entry[j]);
					// list.append(entry);
					if (hbsdcontrol_verbose_flag) {
						printf("\tfound attribute: %s\n", pax_features[i].entry[j]);
					} 
				}

			}
		}

		pos += len;
	}


	*features = NULL; // XXXOP

out:
	free(data);

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
hbsdcontrol_list_features(const char *file, char **features)
{
	int i, j;
	int error;
	char *attrs;
	char *attr; // XXXOP

	error = 0;

	hbsdcontrol_list_extattrs(file, &attrs);
	// XXXOP error handling
	
	if (attrs == NULL)
		err(-1, "attrs == NULL");

	for (attr = attrs ; attr != NULL; attr++) {
		for (i = 0; pax_features[i].feature != NULL; i++) {
			for (j = 0; j < 2; j++) {
				if (!strcmp(pax_features[i].entry[j], attr)) {
					// features.append(strcmp(pax_features[i].entry[j]e);
				}
			}
		}
	}

	free(attrs);

	return (error);
}

int
hbsdcontrol_set_verbose(const int level)
{

	hbsdcontrol_verbose_flag = level;

	return (hbsdcontrol_verbose_flag);
}

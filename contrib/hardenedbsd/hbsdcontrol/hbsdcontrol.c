/*-
 * Copyright (c) 2015-2017 Oliver Pinter <oliver.pinter@HardenedBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/sbuf.h>
#include <sys/uio.h>
#include <sys/extattr.h>

#include <assert.h>
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


static int hbsdcontrol_validate_state(struct pax_feature_state *feature_state);

static int hbsdcontrol_verbose_flag;

const struct pax_feature_entry pax_features[] = {
	{
		.feature = "aslr",
		.extattr = {
			[disable] = "hbsd.pax.noaslr",
			[enable]  = "hbsd.pax.aslr",
		},
	},
	{
		.feature = "segvguard",
		.extattr = {
			[disable] = "hbsd.pax.nosegvguard",
			[enable]  = "hbsd.pax.segvguard",
		},
	},
	{
		.feature = "pageexec",
		.extattr = {
			[disable] = "hbsd.pax.nopageexec",
			[enable]  = "hbsd.pax.pageexec",
		},
	},
	{
		.feature = "mprotect",
		.extattr = {
			[disable] = "hbsd.pax.nomprotect",
			[enable]  = "hbsd.pax.mprotect",
		},
	},
	{
		.feature = "shlibrandom",
		.extattr = {
			[disable] = "hbsd.pax.noshlibrandom",
			[enable]  = "hbsd.pax.shlibrandom",
		},
	},
	{
		.feature = "disallow_map32bit",
		.extattr = {
			[disable] = "hbsd.pax.nodisallow_map32bit",
			[enable]  = "hbsd.pax.disallow_map32bit",
		},
	},
	/* Terminating NULL entry, DO NOT REMOVE! */
	{NULL, {0, 0}}
};

	
int
hbsdcontrol_set_extattr(const char *file, const char *attr, const int val)
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

	len = extattr_set_file(file, attrnamespace, attr,
	    sbuf_data(attrval), sbuf_len(attrval));
	if (len >= 0 && hbsdcontrol_verbose_flag)
		warnx("%s: %s@%s = %s", file, "system", attr, sbuf_data(attrval));

	sbuf_delete(attrval);

	if (len == -1) {
		perror(__func__);
		errx(-1, "abort");
	}

	return (0);
}

int
hbsdcontrol_get_extattr(const char *file, const char *attr, int *val)
{
	int	error;
	int	len;
	int	attrnamespace;
	char	*attrval = NULL;

	if (val == NULL) {
		err(-1, "%s", "val");
	}

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	len = extattr_get_file(file, attrnamespace, attr, NULL, 0);
	if (len == -1) {
		perror(__func__);
		errx(-1, "abort");
	}

#if 0
	if (len >= 0 && hbsdcontrol_verbose_flag)
		warnx("%s: %s@%s = %s", file, "system", attr, sbuf_data(attrval));
#endif

	attrval = calloc(sizeof(char), len);
	if (attrval == NULL) {
		perror(__func__);
		errx(-1, "abort");
	}

	len = extattr_get_file(file, attrnamespace, attr, attrval, len);
	if (len == -1) {
		perror(__func__);
		errx(-1, "abort");
	}

	// XXXOP
	*val = *attrval - '0';

	free(attrval);

	return (0);
}


int
hbsdcontrol_rm_extattr(const char *file, const char *attr)
{
	int error;
	int attrnamespace;

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	if (hbsdcontrol_verbose_flag)
		printf("reset attr: %s on file: %s\n", attr, file);

	error = extattr_delete_file(file, attrnamespace, attr);

	return (error);
}


int
hbsdcontrol_list_extattrs(const char *file, char ***attrs)
{
	int error;
	int attrnamespace;
	ssize_t nbytes;
	char *data;
	size_t pos;
	uint8_t len;
	int fpos;

	nbytes = 0;
	data = NULL;
	pos = 0;
	fpos = 0;

	if (attrs == NULL)
		err(-1, "%s", "attrs");

	error = extattr_string_to_namespace("system", &attrnamespace);
	if (error)
		err(-1, "%s", "system");

	if (hbsdcontrol_verbose_flag)
		printf("list attrs on file: %s\n", file);

	nbytes = extattr_list_file(file, attrnamespace, NULL, 0);
	if (nbytes < 0) {
		error = EFAULT;
		goto out;
	}

	data = calloc(sizeof(char), nbytes);
	if (data == NULL) {
		error = ENOMEM;
		goto out;
	}

	*attrs = (char **)calloc(sizeof(char *), nitems(pax_features) * nitems(pax_features[0].extattr));
	if (*attrs == NULL) {
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

		assert(fpos < nitems(pax_features) * nitems(pax_features[0].extattr));

		/* see EXTATTR(2) about the data structure */
		len = data[pos++];

		/* Yes, this isn't an optimized algorithm. */
		for (i = 0; pax_features[i].feature != NULL; i++) {
			/* The 2 comes from enum pax_attr_state's size */
			for (j = 0; j < 2; j++) {
				attr_len = strlen(pax_features[i].extattr[j]);
				if (attr_len != len) {
					/* Fast path, skip if the size of attribute differs. */
					continue;
				}

				if (!memcmp(pax_features[i].extattr[j], &data[pos], attr_len)) {
					if (hbsdcontrol_verbose_flag) {
						printf("\tfound attribute: %s\n", pax_features[i].extattr[j]);
					}
					(*attrs)[fpos] = strdup(pax_features[i].extattr[j]);
					fpos++;
				}
			}
		}

		pos += len;
	}

	/* NULL terminate the attrs array. */
	(*attrs)[fpos] = NULL;

out:
	free(data);

	return (error);
}

int
hbsdcontrol_set_feature_state(const char *file, const char *feature, enum feature_state state)
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

			error = hbsdcontrol_set_extattr(file, pax_features[i].extattr[disable], !state);
			error |= hbsdcontrol_set_extattr(file, pax_features[i].extattr[enable], state);

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
			error = hbsdcontrol_rm_extattr(file, pax_features[i].extattr[disable]);
			error |= hbsdcontrol_rm_extattr(file, pax_features[i].extattr[enable]);

			break;
		}
	}

	return (error);
}


int
hbsdcontrol_get_all_feature_state(const char *file, struct pax_feature_state **feature_states)
{
	int i, j, k, l;
	int error;
	char **attrs;
	int val;
	bool found = false;

	error = 0;
	attrs = NULL;
	l = 0;

	assert(feature_states != NULL);

	*feature_states = calloc(sizeof(struct pax_feature_state), nitems(pax_features));

	assert(*feature_states != NULL);

	hbsdcontrol_list_extattrs(file, &attrs);
	if (attrs == NULL)
		err(-1, "attrs == NULL");

	/*
	 * Ezt itt kellene optimalizalni, hogy az atts legyen kivul,
	 * viszont csak akkor kellene novelni az l-t ha ..
	 */
	for (j = 0; pax_features[j].feature != NULL; j++) {
		for (i = 0; attrs[i] != NULL; i++) {
			for (k = 0; k < 2; k++) {
				if (!strcmp(pax_features[j].extattr[k], attrs[i])) {

					hbsdcontrol_get_extattr(file, attrs[i], &val);
					if (hbsdcontrol_verbose_flag)
						printf("%s (%s: %d)\n", pax_features[j].feature, attrs[i], val);

					assert(l < nitems(pax_features));

					if ((*feature_states)[l].feature == NULL)
						(*feature_states)[l].feature = strdup(pax_features[j].feature);
					(*feature_states)[l].internal[k].state = val;	
					(*feature_states)[l].internal[k].extattr = strdup(pax_features[j].extattr[k]);
					(*feature_states)[l].state = hbsdcontrol_validate_state(&(*feature_states)[l]);
					found = true;
				}
			}
		}
		l = found ? l+1 : l;
		found = false;
	}

	for (i = 0; attrs[i] != NULL; i++) {
		free(attrs[i]);
		attrs[i] = NULL;
	}
	free(attrs);
	attrs = NULL;

	return (error);
}


int
hbsdcontrol_list_features(const char *file, char **features)
{
	struct pax_feature_state	*feature_states;
	int i, j;

	hbsdcontrol_get_all_feature_state(file, &feature_states);

	for (i = 0; i < nitems(pax_features); i++) {
		if (feature_states[i].feature == NULL)
			continue;

		printf("%s\t", feature_states[i].feature);
		for (j = 0; j < 2; j++) {
			printf("%s: %d\t", feature_states[i].internal[j].extattr,
			    feature_states[i].internal[j].state);
		}
		printf("->\t%d\n", feature_states[i].state);
	}

	// factor this out to pax_feature_states_free(struct pax_feature_states **ctx)
	for (i = 0; i < nitems(pax_features); i++) {
		free(feature_states[i].feature);
		feature_states[i].feature = NULL;
		for (j = 0; j < 2; j++) {
			free(feature_states[i].internal[j].extattr);
			feature_states[i].internal[j].extattr = NULL;
		}
	}

	return (0);
}

static int
hbsdcontrol_validate_state(struct pax_feature_state *feature_state)
{
	int state = 3;
	int nf, f;

	static int	state_map[4][3] = {
				// nf	    f
		{0, 0, 0},	// false, false -> disable / conflict*
		{0, 1, 0},	// false, true  -> enable
		{1, 0, 0},	// true,  false -> disable
		{1, 1, 2},	// true,  true  -> conflict
		
		/* *
		 * disable if one of them missing
		 * and conflict when both present
		 */
	};

	assert(feature_state != NULL);

	nf = feature_state->internal[disable].state;
	f = feature_state->internal[enable].state;

	if (nf == 0 && f == 0)
		state = 2;
	else if (nf == 0 && f == 1)
		state = 1;
	else if (nf == 1 && f == 0)
		state = 0;
	else if (nf == 1 && f == 1)
		state = 2;
	else
		assert(false);

	return (state);
}

int
hbsdcontrol_set_verbose(const int level)
{

	hbsdcontrol_verbose_flag = level;

	return (hbsdcontrol_verbose_flag);
}

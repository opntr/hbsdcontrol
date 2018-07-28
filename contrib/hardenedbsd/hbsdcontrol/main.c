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

/*
 * Warning: this file currently is just a stub, to test libhbsdcontrol!
 */

#include <sys/types.h>
#include <sys/sbuf.h>
#include <sys/uio.h>
#include <sys/extattr.h>
#include <sys/stat.h>

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

bool flag_force = false;
int flag_verbose = 0;
bool flag_immutable = false;
bool flag_keepgoing = false;

static void usage(void);

static int pax_cb(int *argc, char ***argv);
static void pax_usage(bool terminate);
static int pax_enable_cb(int *argc, char ***argv);
static int pax_disable_cb(int *argc, char ***argv);
static int pax_reset_cb(int *argc, char ***argv);
static int pax_list_cb(int *argc, char ***argv);

static int dummy_cb(int *argc, char ***argv);


struct hbsdcontrol_action_entry {
	const char	*action;
	const int	 min_argc;
	int		(*fn)(int *, char ***);
};

struct hbsdcontrol_command_entry {
	const char	*cmd;
	const int	 min_argc;
	int		(*fn)(int *, char ***);
	void		(*usage)(bool);
};

const struct hbsdcontrol_command_entry hbsdcontrol_commands[] = {
	{"pax",		3,	pax_cb,	pax_usage},
	{NULL,		0,	NULL,	NULL},
};

const struct hbsdcontrol_action_entry hbsdcontrol_pax_actions[] = {
	{"enable",	3,	pax_enable_cb},
	{"disable",	3,	pax_disable_cb},
	{"status",	3,	dummy_cb},
	{"reset",	3,	pax_reset_cb},
	{"reset-all",	2,	dummy_cb},
	{"list",	2,	pax_list_cb},
	{NULL,		0,	NULL}
};

static int
dummy_cb(int *argc, char ***argv)
{

	errx(-1, "dummy_cb");
}

static int
enable_disable(int *argc, char ***argv, int state)
{
	char *feature;
	char *file;
	struct stat st;

	if (*argc < 3)
		err(-1, "bar");


	feature = (*argv)[1];
	file = (*argv)[2];

	*argc -= 2;
	*argv += 2;

	if (lstat(file, &st)) {
		fprintf(stderr, "missing file: %s\n", file);
		return (1);
	}

	hbsdcontrol_set_feature_state(file, feature, state);

	return (0);
}

static int
pax_list(int *argc, char ***argv)
{
	char *feature;
	char *file;
	char *dummy;
	struct stat st;

	if (*argc < 2)
		err(-1, "bar");


	file = (*argv)[1];

	(*argc)--;
	(*argv)--;

	if (lstat(file, &st)) {
		fprintf(stderr, "missing file: %s\n", file);
		return (1);
	}


	hbsdcontrol_list_features(file, &dummy);

	return (0);
}

static int
pax_enable_cb(int *argc, char ***argv)
{

	return (enable_disable(argc, argv, enable));
}

static int
pax_disable_cb(int *argc, char ***argv)
{

	return (enable_disable(argc, argv, disable));
}

static int
pax_rm_fsea(int *argc, char ***argv)
{
	int i;
	int error;
	char *feature;
	char *file;

	if (*argc < 3)
		err(-1, "baz");

	feature = (*argv)[1];
	file = (*argv)[2];

	(*argc) -= 2;
	*argv += 2;

	return (hbsdcontrol_rm_feature_state(file, feature));
}

static int
pax_reset_cb(int *argc, char ***argv)
{

	return (pax_rm_fsea(argc, argv));
}

static int
pax_list_cb(int *argc, char ***argv)
{

	return (pax_list(argc, argv));
}

static void
pax_usage(bool terminate)
{
	int i;

	fprintf(stderr, "usage:\n");
	for (i = 0; hbsdcontrol_pax_actions[i].action != NULL; i++) {
		if (hbsdcontrol_pax_actions[i].min_argc == 2)
			fprintf(stderr, "\thbsdcontrol pax %s file\n",
			    hbsdcontrol_pax_actions[i].action);
		else
			fprintf(stderr, "\thbsdcontrol pax %s feature file\n",
			    hbsdcontrol_pax_actions[i].action);
	}

	if (terminate)
		exit(-1);
}

static int
pax_cb(int *argc, char ***argv)
{
	int i;

	if (*argc < 2)
		return (1);

	for (i = 0; hbsdcontrol_pax_actions[i].action != NULL; i++) {
		if (!strcmp(*argv[0], hbsdcontrol_pax_actions[i].action)) {
			if (*argc < hbsdcontrol_pax_actions[i].min_argc)
				pax_usage(true);

			return (hbsdcontrol_pax_actions[i].fn(argc, argv));
		}
	}

	return (1);
}



static void
usage(void)
{
	int i;

	for (i = 0; hbsdcontrol_commands[i].cmd != NULL; i++) {
		hbsdcontrol_commands[i].usage(false);
	}

	exit(-1);
}


int
main(int argc, char **argv)
{
	int i;
	int ch;

	while ((ch = getopt(argc, argv, "fhikv")) != -1) {
		switch (ch) {
		case 'f':
			flag_force = true;
			break;
		case 'i':
			flag_immutable = true;
			break;
		case 'v':
			flag_verbose < 3 ? flag_verbose++ : 0;
			hbsdcontrol_set_verbose(flag_verbose);
			break;
		case 'k':
			flag_keepgoing = true;
			break;
		case 'h':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (getuid() != 0) {
		errx(-1, "!root");
	}

	while (argc > 0) {
		for (i = 0; hbsdcontrol_commands[i].cmd != NULL; i++) {
			if (!strcmp(argv[0], hbsdcontrol_commands[i].cmd)) {
				argv++;
				argc--;

				if (hbsdcontrol_commands[i].fn(&argc, &argv) != 0) {
					if (hbsdcontrol_commands[i].usage)
						hbsdcontrol_commands[i].usage(flag_keepgoing ? false : true);
				}
			}
		}

		argv++;
		argc--;
	}

	if (flag_verbose > 0)
		printf("argc at the end: %i\n", argc);

	return (0);
}


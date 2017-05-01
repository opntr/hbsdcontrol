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

bool flag_force = false;
int flag_verbose = 0;
bool flag_immutable = false;

static int dummy_cb(int argc, char **argv);
static int enable_cb(int argc, char **argv);
static int disable_cb(int argc, char **argv);
static int reset_cb(int argc, char **argv);

struct hbsdcontrol_command_entry {
	const char	*cmd;
	const int	 min_argc;
	int		(*fn)(int, char **);
};

const struct hbsdcontrol_command_entry hbsdcontrol_commands[] = {
	{"enable",	3,	enable_cb},
	{"disable",	3,	disable_cb},
	{"status",	3,	dummy_cb},
	{"reset",	3,	reset_cb},
	{"reset-all",	2,	dummy_cb},
	{"list",	2,	dummy_cb},
	{NULL,		0,	NULL}
};

static int
dummy_cb(int argc, char **argv)
{

	errx(-1, "dummy_cb");
}

static int
enable_disable(int argc, char **argv, int state)
{
	int i;
	char *feature;
	char *file;

	if (argc < 3)
		err(-1, "bar");

	feature = argv[1];
	file = argv[2];

	hbsdcontrol_set_feature_state(file, feature, state);

	return (0);
}

static int
enable_cb(int argc, char **argv)
{

	return (enable_disable(argc, argv, enable));
}

static int
disable_cb(int argc, char **argv)
{

	return (enable_disable(argc, argv, disable));
}

static int
rm_fsea(int argc, char **argv)
{
	int i;
	int error;
	char *feature;
	char *file;

	if (argc < 3)
		err(-1, "baz");

	feature = argv[1];
	file = argv[2];

	for (i = 0; pax_features[i].feature != NULL; i++) {
		if (!strcmp(pax_features[i].feature, feature)) {
			if (flag_verbose) 
				printf("reset %s on %s\n",
				    pax_features[i].feature, file);
			hbsdcontrol_rm_extattr(file, pax_features[i].entry[disable]);
			hbsdcontrol_rm_extattr(file, pax_features[i].entry[enable]);
		}
	}

	return (0);
}

static int
reset_cb(int argc, char **argv)
{

	return (rm_fsea(argc, argv));
}


static void __dead2
usage(void)
{
	int i;

	fprintf(stderr, "usage:\n");
	for (i = 0; hbsdcontrol_commands[i].cmd != NULL; i++) {
		if (hbsdcontrol_commands[i].min_argc == 2)
			fprintf(stderr, "\thbsdcontrol %s file\n",
			    hbsdcontrol_commands[i].cmd);
		else
			fprintf(stderr, "\thbsdcontrol %s feature file\n",
			    hbsdcontrol_commands[i].cmd);
	}

	exit(-1);
}

int
main(int argc, char **argv)
{
	int i;
	int ch;

	while ((ch = getopt(argc, argv, "fhiv")) != -1) {
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
				if (argc < hbsdcontrol_commands[i].min_argc)
					usage();
				hbsdcontrol_commands[i].fn(argc, argv);
				argc -= hbsdcontrol_commands[i].min_argc;
				argv += hbsdcontrol_commands[i].min_argc;
				
				return (0);
			}
		}

		argc--;
		argv++;
	}

	printf("%i\n", argc);

	return (0);
}


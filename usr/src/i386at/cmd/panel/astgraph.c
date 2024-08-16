/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:astgraph.c	1.4"

/*
 * astgraph.c - controls the front panel CPU graph display
 *	on the AST Manhattan MP
 *	Three modes:
 *		1)  histogram: displays CPU utilization
 *		2)  status: displays which processors are in use
 *		3)  online: displays which processors are online
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>		/* string manipulations, e.g., translate */

#include <sys/types.h>		/* primitive system data types */
#include <sys/stat.h>		/* stat data types */
#include <fcntl.h>		/* file control */
#include <errno.h>		/* error handling */
#include <sys/errno.h>		/* error handling */
#include <sys/ebi.h>		/* defines the AST EBI II interface */

#include <unistd.h>		/* header file for symbolic constants */

#define	STRSIZE	80

void read_config_file();
void open_ioctl_dev();
void set_hist_mode();
void set_status_mode();
void set_online_mode();
void get_existing_mode();
void checkuid();

int	fd;			/* file descriptor */
char	device[] = "/dev/astm";	/* device */
char	program[] = "astgraph";		/* pointer to the program name */
char	err_str[STRSIZE];	/* string to build errors passed to perror */

int	hist_flag = 0;	 	/* histogram mode? - CPU utilization */
int	status_flag = 0; 	/* status mode? - which processors are being used */
int	online_flag = 0;	/* online mode? - which processors are on-line */
int	quiet_flag = 0;		/* are we in quiet mode? */
int	config_flag = 0;	/* has a configuration file been supplied? */
FILE	*config_file;		/* file pointer for config file */

/* modes for config file */
#define EXISTING_MODE	0 
#define HIST_MODE	1 
#define STATUS_MODE	2 
#define ONLINE_MODE	3

int	graph_mode = EXISTING_MODE;	/* default graph mode -- existing */

/* tokens for config file */
#define GRAPH_MODE	0

#define MAX_TOKENS		1

char *KEYWORD_TBL[] =
{
"GRAPH_MODE"
};

/* strings to do the lower- to upper-case translation */
char tr1[] = "abcdefghijklmnopqrstuvwxyz-";
char tr2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

void main(argc,argv)
int argc;
char *argv[];
{
        int c, errflg = 0;
	char options_string[] = "hsoqc:?";
        extern char     *optarg;
        extern int      optind;
	char	sys_string[STRSIZE];

	/* save program name */
	/*program = argv[0];*/

	checkuid();

        while ((c = getopt(argc, argv, options_string)) != -1) {
                switch (c) {
		case 'c' :
                        /* look for config file */
                        if ((config_file = fopen(optarg, "r")) == NULL)
			{
				sprintf(err_str, "%s: %s", program, optarg);
				perror(err_str);
			}
                        else
				++config_flag;
                        break;
		case 'h' :
			/* set flags */
			++hist_flag;
			break;
		case 's' :
			/* set flags */
			++status_flag;
			break;
		case 'o' :
			/* set flags */
			++online_flag;
			break;
		case 'q' :
			/* set flags */
			++quiet_flag;
			break;
                case '?':
                        ++errflg;
                }
        }

	if (hist_flag + status_flag + online_flag > 1)
	{
		fprintf(stderr, "%s: Select only one graph mode\n", program);
		++errflg;
	}

	/* check number of arguments */
        if (argc - optind > 0)
                ++errflg;

        if (errflg) {
                giveusage();
                exit(2);
        }

	if (config_flag)
		read_config_file();

        /* open the ioctl device to do ioctl calls */
        open_ioctl_dev();

	/* Act on the flags */
	if (hist_flag)
		set_hist_mode();
	else if (status_flag)
		set_status_mode();
	else if (online_flag)
		set_online_mode();
	else
		get_existing_mode();

	exit(0);
}

/*
 * Giveusage ()
 * Give a concise message on how to use this program
 */
giveusage()
{
        fprintf(stderr, "Usage: %s [-h | -s | -o] [-c configfile] [-q]\n", program);
        fprintf(stderr,
"	-h       CPU histogram - monitor CPU utilization\n\
	-s       CPU status - shows which CPUs are in use\n\
	-o       CPU on-line - shows which CPUs are on-line\n\
	-c file  reads from configuration file\n\
	-q       quiet mode; only errors are reported\n\
  With no arguments, reports the current bar graph mode\n");
}

void checkuid()
{
	if (((int)getuid()) != 0)
	{
                fprintf(stderr, "%s: Must be root to use\n", program);
                exit(1);
	}
}

/*
 *********************  READ CONFIG FILE
 */

void read_config_file()
{
	int token;
	char input_line[STRSIZE];
	char keyword[STRSIZE];
	char value[STRSIZE];

	while (fgets(input_line, BUFSIZ, config_file) != NULL)
	{
		/* scan line, skipping blank, incomplete, or comment lines */
		if ((sscanf(input_line, "%s %[^#\n]\n", keyword, value) < 2) ||
		   (keyword[0] == '#'))
			continue;

		/* tokenize the keyword, and check for validity */
		if ((token = tokenize(keyword)) == -1)
			continue;
		/* now do something with "value" based on the keyword token */
		switch (token)
		{
		case GRAPH_MODE:
			if (atoi(value) == HIST_MODE ||
			   (value[0] == 'H') ||
			   (value[0] == 'h'))
				++hist_flag;
			else
			if (atoi(value) == STATUS_MODE ||
			   (value[0] == 'S') ||
			   (value[0] == 's'))
				++status_flag;
			else
			if (atoi(value) == ONLINE_MODE ||
			   (value[0] == 'O') ||
			   (value[0] == 'o'))
				++online_flag;
			break;
		}
	}
}

int tokenize(char *keyword)
{
	int token;
	char tr_keyword[STRSIZE];

	/* convert lower case to upper, dashes to underscores */
	/* config file is as forgiving as possible */
	strtrns(keyword, tr1, tr2, tr_keyword);
	for (token=0; token < MAX_TOKENS; token++)
	{
		if (strcmp(tr_keyword, KEYWORD_TBL[token]) == 0)
			return (token);
	}
	return(-1);
}


/*
 *********************  OPEN IOCTL DEV
 */

void open_ioctl_dev()
{
        if ((fd = open(device, O_RDWR)) == -1)
        {
		fprintf(stderr, "%s: Cannot open %s device\n", program, device);
                fprintf(stderr, "%s: Unable to report on or change front panel graph\n",
                        program);
                exit(1);
        }
}

/*
 *********************  SET HIST MODE
 */

void set_hist_mode()
{
	int graph_contents;

	graph_contents = 0;

	if (ioctl(fd, EBI_SET_BAR_CONTENTS, &graph_contents) == -1)
	{
		perror(program);
		exit(1);
	}

	graph_mode = PANEL_MODE_HISTOGRAM;

	if (ioctl(fd, EBI_SET_BAR_GRAPH_MODE, &graph_mode) == -1)
	{
		if (errno == ENOMSG)
			fprintf(stderr,"%s: Mode not supported by current OS\n", program);
		else
			perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel CPU graph set to histogram mode\n");

	graph_contents = 0;
}

/*
 *********************  SET STATUS MODE
 */

void set_status_mode()
{
	int graph_contents;

	graph_contents = 0;

	if (ioctl(fd, EBI_SET_BAR_CONTENTS, &graph_contents) == -1)
	{
		perror(program);
		exit(1);
	}

	graph_mode = PANEL_MODE_STATUS;

	if (ioctl(fd, EBI_SET_BAR_GRAPH_MODE, &graph_mode) == -1)
	{
		if (errno == ENOMSG)
			fprintf(stderr,"%s: Mode not supported by current OS\n", program);
		else
			perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel CPU graph set to status mode\n");
}


/*
 *********************  SET ONLINE MODE
 */

void set_online_mode()
{
	int graph_contents;

	graph_contents = 0;

	if (ioctl(fd, EBI_SET_BAR_CONTENTS, &graph_contents) == -1)
	{
		if (errno == ENOMSG)
			fprintf(stderr,"%s: Mode not supported by current OS\n", program);
		else
			perror(program);
		exit(1);
	}

	graph_mode = PANEL_MODE_ONLINE;

	if (ioctl(fd, EBI_SET_BAR_GRAPH_MODE, &graph_mode) == -1)
	{
		if (errno == ENOMSG)
			fprintf(stderr,"%s: Mode not supported by current OS\n", program);
		else
			perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel CPU graph set to online mode\n");
}

/*
 *********************  GET EXISTING MODE
 */

void get_existing_mode()
{
	if (ioctl(fd, EBI_GET_BAR_GRAPH_MODE, &graph_mode) == -1)
	{
		perror(program);
		exit(1);
	}

	fprintf(stderr,"Front panel CPU graph is in ", program);

	switch (graph_mode)
	{
	case PANEL_MODE_HISTOGRAM:
		fprintf(stderr,"histogram mode\n");
		break;
	case PANEL_MODE_STATUS:
		fprintf(stderr,"status mode\n");
		break;
	case PANEL_MODE_OVERRIDE:
		fprintf(stderr,"override mode\n");
		break;
	case PANEL_MODE_ONLINE:
		fprintf(stderr,"online mode\n");
		break;
	default:
		fprintf(stderr,"an undefined mode\n");
		break;
	}
}

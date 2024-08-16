/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:astcache.c	1.2"

/*
 * astgraph.c - controls the cache on the AST Manhattan MP
 *	Two fuctions:
 *		1)  enable RAM cache
 *		2)  disable RAM cache
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
void turn_cache_on();
void turn_cache_off();
void get_info();
void checkuid();

int	fd;				/* file descriptor */
char	device[] = "/dev/astm";		/* device */
char	program[] = "astcache";		/* pointer to the program name */
char	err_str[STRSIZE];		/* string to build errors passed to perror */

int	enable_flag = 0;	 	/* are we turninc cache on? */
int	disable_flag = 0; 		/* are we turninc cache off? */
int	quiet_flag = 0;		/* are we in quiet mode? */
int	config_flag = 0;	/* has a configuration file been supplied? */
FILE	*config_file;		/* file pointer for config file */

int	graph_mode = RAM_CACHE_ENABLE;	/* default graph mode -- existing */

/* modes for config file */
#define CACHE_OFF		0
#define CACHE_ON		1

/* tokens for config file */
#define CACHE_MODE		0

#define MAX_TOKENS		1

char *KEYWORD_TBL[] =
{
"CACHE_MODE"
};

/* strings to do the lower- to upper-case translation */
char tr1[] = "abcdefghijklmnopqrstuvwxyz-";
char tr2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

void main(argc,argv)
int argc;
char *argv[];
{
        int c, errflg = 0;
	char options_string[] = "nfqc:?";
        extern char     *optarg;
        extern int      optind;
	char	sys_string[STRSIZE];

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
		case 'n' :
			++enable_flag;
			break;
		case 'f' :
			++disable_flag;
			break;
		case 'q' :
			/* set flags */
			++quiet_flag;
			break;
                case '?':
                        ++errflg;
                }
        }

	if (enable_flag && disable_flag)
	{
		fprintf(stderr, 
			"%s: -n and -f options are mutually exclusive\n",
			program);
		++errflg;
	}

#if 0
	if (! (enable_flag || disable_flag))
	{
		++errflg;
	}
#endif

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
	if (enable_flag)
		turn_cache_on();
	else if (disable_flag)
		turn_cache_off();
#if 0
	else
		get_info();
#endif

	exit(0);
}

/*
 * Giveusage ()
 * Give a concise message on how to use this program
 */
giveusage()
{
        fprintf(stderr, "Usage: %s [-n | -f] [-c configfile] [-q]\n", program);
        fprintf(stderr,
"	-n       turn cache on\n\
	-f       turn cache off\n\
	-c file  reads from configuration file\n\
	-q       quiet mode; only errors are reported\n");
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
		case CACHE_MODE:
			if ((value[0] == 'N') ||
			   (value[0] == 'n'))
				++enable_flag;
			else if ((value[0] == 'F') ||
			   (value[0] == 'f'))
				++disable_flag;
			else if (atoi(value) == CACHE_ON)
				++enable_flag;
			else if (atoi(value) == CACHE_OFF)
				++disable_flag;
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
                fprintf(stderr, "%s: Unable to turn cache on or off\n",
                        program);
                exit(1);
        }
}

/*
 *********************  TURN CACHE ON
 */

void turn_cache_on()
{
	int cache_mode = RAM_CACHE_ENABLE;

	if (ioctl(fd, EBI_SET_RAM_CACHE, &cache_mode) == -1)
	{
		perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"RAM cache enabled\n");
}

/*
 *********************  TURN CACHE OFF
 */

void turn_cache_off()
{
	int cache_mode = RAM_CACHE_DISABLE;

	if (ioctl(fd, EBI_SET_RAM_CACHE, &cache_mode) == -1)
	{
		perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"RAM cache disabled\n");
}


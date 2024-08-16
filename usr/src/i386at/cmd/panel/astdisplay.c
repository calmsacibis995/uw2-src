/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:astdisplay.c	1.4"

/*
 * astdisplay.c - manipulates the front panel alpha-numeric display
 *	on the AST Manhattan MP
 *	Three modes:
 *		1)  display text (length <= size of display)
 *		2)  scroll text (length > size of display)
 *		3)  show CPU utilization as a percentage
 */

/* a non-zero value compiles a version that leaves 
 * scrolling in the foreground */
#define SCRFORE	1

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>		/* string manipulations, e.g. translate */

#include <sys/types.h>		/* primitive system data types */
/*#include <sys/stat.h>*/		/* stat data types */
#include <fcntl.h>		/* file control */
#include <sys/errno.h>
#include <sys/ebi.h>		/* defines the AST EBI II interface */
#include <time.h>		/* include files for time libraries */

#include <unistd.h>		/* header file for symbolic constants */

#define	STRSIZE	80
#define BIGSIZE 512

void set_fixed_mode();
void set_scroll_mode();
void set_util_mode();
void get_info();
void read_config_file();
void open_ioctl_dev();
void initScrData();
void checkuid();

int	fd;			/* file descriptor */
char	device[] = "/dev/astm";	/* device */
char	program[] = "astdisplay"; 	/* pointer to the program name */
char	err_str[STRSIZE];	/* string for building error messages for perror */

int	delay_flag = 0;		/* have we provided a delay? */
int	pause_flag = 0;  	/* have we specified a pause? */
int	text_flag = 0;	 	/* have we specified text mode? */
int	util_flag = 0;	 	/* have we specified utilization mode? */
int     quiet_flag = 0;         /* are we in quiet mode? */
int	config_flag = 0;	/* has a configuration file been supplied? */
FILE	*config_file;		/* file pointer for config file */

long	scroll_delay = 250;	/* scrolling delay (in milliseconds) */
long	scroll_pause = 500;	/* pause between scrolling (in milliseconds) */

char	display_text[BIGSIZE] = "";
char	tmp_text[BIGSIZE] = "";

int	disp_size = 4;

/* the configuration file stuff */

#define UTIL_MODE		1
#define TEXT_MODE		2

#define DISPLAY_MODE		0
#define DISPLAY_TEXT 		1
#define SCROLL_DELAY		2
#define SCROLL_PAUSE		3

#define MAX_TOKENS		4

char *KEYWORD_TBL[] =
{
"DISPLAY_MODE",
"DISPLAY_TEXT",
"SCROLL_DELAY",
"SCROLL_PAUSE"
};

char tr1[] = "abcdefghijklmnopqrstuvwxyz-";
char tr2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

void main(argc,argv)
int argc;
char *argv[];
{
        int c, errflg = 0;
	char options_string[] = "d:w:t:uqc:?";
        extern char     *optarg;
        extern int      optind;
	char	sys_string[STRSIZE];
	long	tmp_delay, tmp_pause;

	checkuid();

        while ((c = getopt(argc, argv, options_string)) != -1) {
                switch (c) {
		case 'c' :
                        /* look for config file */
                        if ((config_file = fopen(optarg, "r")) == NULL)
			{
                                sprintf(err_str, "%s: %s", program, optarg);
				perror(err_str);
				exit(1);
			}
                        else
				++config_flag;
                        break;
		case 'u' :
			/* set flags */
			++util_flag;
			break;
		case 't' :
                        /* argument is text to place on front panel */
			/* note: we use tmp vars to give the config file a crack */
			/* at setting the vars before setting them w/ command line */
			/* values */
			strcpy(tmp_text, optarg);
			++text_flag;
                        break;
		case 'd' :
                        /* argument is scrolling delay (in milliseconds) */
			/* note: we use tmp vars to give the config file a crack */
			/* at setting the vars before setting them w/ command line */
			/* values */
			tmp_delay = atol(optarg);
			++delay_flag;
                        break;
		case 'w' :
                        /* argument is pause between scrolling (in milliseconds) */
			/* note: we use tmp vars to give the config file a crack */
			/* at setting the vars before setting them w/ command line */
			/* values */
			tmp_pause = atol(optarg);
			++pause_flag;
                        break;
		case 'q' :
			/* set flags */
			++quiet_flag;
			break;
                case '?':
                        ++errflg;
                }
        }

	/* check number of arguments */
        if (argc - optind > 0)
                ++errflg;

	/* check incompatible options and arguments */
	if (util_flag &&
	   (text_flag))
	{
		fprintf(stderr, "%s: Select only one display mode\n",
			program);
		++errflg;
	}

        if (errflg) {
                giveusage();
                exit(2);
        }

	/* read config files, set values */
	/* note: command line values override config file */
	if (config_flag)
		read_config_file();

	/* open the ioctl device to do ioctl calls */
	open_ioctl_dev();

	/* get the size of the front panel display */
	disp_size = get_disp_size();

	/* if it was supplied, deal with display text */
	if (text_flag)
	{
		strcpy(display_text, tmp_text);
	}

	/* if it was supplied on the command line, deal with scroll delay */
	if (delay_flag)
	{
		scroll_delay = tmp_delay;
	}

	/* if it was supplied on the command line, deal with scroll pause */
	if (pause_flag)
	{
		scroll_pause = tmp_pause;
	}


	if (util_flag)
	{
		set_util_mode();
	}
	else if (text_flag)
	{
		if (strlen(display_text) <= disp_size)
		{
			set_fixed_mode();
		}
		else
		{
			set_scroll_mode();
		}
	}
	/* if no options were given, display info on display */
	else
	{
		get_info();
	}

        exit(0);
}

/*
 * Giveusage ()
 * Give a concise message on how to use this program
 */
giveusage()
{
        fprintf(stderr,
"Usage:  %s -t \"text\" [-m delay] [-w wait] [-c file] [-q]\n\
        %s -u [-c file] [-q]\n", program, program);
	fprintf(stderr,
"        -t \"text\"  display text on front panel\n\
        -d delay   delay between movement (in milliseconds)\n\
        -w wait    wait after scrolling (in milliseconds)\n\
        -u         display processor utilization on front panel\n\
        -q         quiet mode; only errors are reported\n\
        -c file    reads from configuration file\n\
  With no arguments, report on current state of display.\n");
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
	char input_line[BUFSIZ];
	char keyword[STRSIZE];
	char value[STRSIZE];

	while (fgets(input_line, BUFSIZ, config_file) != NULL)
	{
		/* start with blank values */
		keyword[0]='\0';
		value[0]='\0';

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
		case DISPLAY_MODE:
			if ((value[0] == 'U') ||
			   (value[0] == 'u'))
				++util_flag;
			else if ((value[0] == 'T') ||
			   (value[0] == 't'))
				++text_flag;
			else if (atoi(value) == UTIL_MODE)
				++util_flag;
			else if (atoi(value) == TEXT_MODE)
				++text_flag;
			break;
		case DISPLAY_TEXT:
			strcpy(tmp_text, value);
			break;
		case SCROLL_DELAY:
			scroll_delay = atol(value);
			break;
		case SCROLL_PAUSE:
			scroll_pause = atol(value);
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
		fprintf(stderr, "%s: Unable to open %s device\n", program, device);
		exit(1);
	}
}

/*
 *********************  GET DISP INFO
 */


int get_disp_size()
{
	struct dispinfo display_info;

	if (ioctl(fd, EBI_GET_ALPHA_INFO, &display_info) == -1)
	{
		perror(program);
		exit(1);
	}

	return(display_info.size);
}

/*
 *********************  FIXED MODE
 */

void set_fixed_mode()
{
	int mode;

	/* if the length of the string is less than the number of bytes to display */
	/* then append spaces to the end */
	if (strlen(display_text) < disp_size)
	{
		int i;

		for (i=strlen(display_text); i < disp_size; i++)
			display_text[i] = ' ';
		display_text[disp_size] = '\0';
	}

	mode=ALPHA_MODE_TEXT;
	if (ioctl(fd, EBI_SET_ALPHA_MODE, &mode) == -1)
	{
		perror(program);
		exit(1);
	}

	if (ioctl(fd, EBI_SET_ALPHA_DISP, display_text) == -1)
	{
		perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel display showing text\n", program);

}


/*
 *********************  SCROLL MODE
 */

void set_scroll_mode()
{
	int mode;
	char *strptr;
	char	new_text[BIGSIZE];
	char	tmp_text[STRSIZE];

	mode=ALPHA_MODE_TEXT;
	if (ioctl(fd, EBI_SET_ALPHA_MODE, &mode) == -1)
	{
		perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel display scrolling\n");

	/* 
	 * pad the beggining of displayed text so the text will scroll on to a 
	 * blank display 
	 */
	strcpy(new_text, "    ");

	/*
	 * copy the display_text to the the new padded string
	 */
	strcat(new_text, display_text);

	/* 
	 * pad the end of display_text so the text will scroll completely off
	 */
	strcat(new_text, "    ");


#ifndef SCRFORE
	/* fork a new process to endlessly scroll */
	switch(fork())
	{
	case 0:
#endif
		while (1)
		{
			for (strptr = new_text; 
			     strptr < new_text + (strlen(new_text) - disp_size + 1); 
			     strptr++)
			{
				strncpy(tmp_text, strptr, disp_size);
				tmp_text[disp_size] = '\0';

				if (ioctl(fd, EBI_SET_ALPHA_DISP, tmp_text) == -1)
				{
					perror(program);
					exit(1);
				}
				#ifdef NONAP
				sleep((int)scroll_delay/1000);
				#else
				nap(scroll_delay);
				#endif
			} 
			#ifdef NONAP
			sleep((int)scroll_pause/1000);
			#else
			nap(scroll_pause);
			#endif
		}
		/* never reached */
#ifndef SCRFORE
	}
#endif
}

/*
 *********************  UTILIZATION MODE
 */

void set_util_mode()
{
	int mode;

	mode = ALPHA_MODE_UTIL;
	if (ioctl(fd, EBI_SET_ALPHA_MODE, &mode) == -1)
	{
		perror(program);
		exit(1);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Front panel display showing utilization\n");
}

/*
 *********************  GET INFO
 */

void get_info()
{
	char	tmp_text[STRSIZE];
	int	mode;

	if (ioctl(fd, EBI_GET_ALPHA_MODE, &mode) == -1)
	{
		perror(program);
		exit(1);
	}
	printf("Front panel display is in ", program);
	switch(mode)
	{
	case ALPHA_MODE_TEXT:
		printf("text mode.\n");
		break;
	case ALPHA_MODE_UTIL:
		printf("utilization mode.\n");
		break;
	default:
		printf("an undefined mode.\n");
		break;
	}
}

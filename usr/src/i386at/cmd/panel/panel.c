/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:panel.c	1.3"

/*
 * panel.c - provides an interface to three utilities for the AST Manhattan
 *	astmonitor - monitors front panel buttons, power and thermal states
 *	astgraph - controls the front panel LED bar graph
 *	astdisplay - controls the front panel LED alphanumeric display
 */


#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>		/* string manipulations, e.g. translate */

#include <sys/types.h>		/* primitive system data types */
#include <fcntl.h>		/* file control */
#include <sys/errno.h>
#include <unistd.h>           /* header file for symbolic constants */

#define	STRSIZE	80

void read_config_file();
void run_monitor();
void run_display();
void run_graph();
void run_cache();
void get_info();
void checkuid();


int	fd;			/* file descriptor */
char	program[] = "astdisplay"; 	/* pointer to the program name */

/* general flags */
int	config_flag = 0;	/* has a configuration file been supplied? */
int	quiet_flag = 0;		/* are we in quiet mode? */

/* astmonitor flags */
int	monitor_flag = 0;	/* have we provided a delay? */

/* astdisplay flags */
int	text_flag = 0;	/* have we specified text to display? */
int	util_flag = 0;	 /* have we specified utilization mode? */

/* astgraph flags */
int	hist_flag = 0;		/* histogram mode? - CPU utilization */
int	status_flag = 0;	/* status mode? - which CPUs are being used */
int	online_flag = 0;	/* online mode? - which CPUs are on-line */

/* astcache flags */
int	enable_flag = 0;	 /* enable RAM cache */
int	disable_flag = 0;	 /* disable RAM cache */

char	general_opt[STRSIZE] = "";	/* option line to pass to all programs */
char	monitor_opt[STRSIZE] = "";	/* option line to pass astmonitor */
char	display_opt[STRSIZE] = "";	/* option line to pass astdisplay */
char	graph_opt[STRSIZE] = "";	/* option line to pass astgraph */
char	cache_opt[STRSIZE] = "";	/* option line to pass astcache */

char    display_text[STRSIZE] = " AST";

void main(argc,argv)
int argc;
char *argv[];
{
        int c, errflg = 0;
	char options_string[] = "c:q?mut:hsofn";
        extern char     *optarg;
        extern int      optind;
	char	sys_string[STRSIZE];
	char	tmp_opt[STRSIZE];
	long	tmp_delay, tmp_pause;

	checkuid();

	/* save program name */
	/*program_name = argv[0];*/

        while ((c = getopt(argc, argv, options_string)) != -1) {
                switch (c) {
		/*
		 * general options
		 */
		case 'c' :
                        /* look for config file */
                        if (fopen(optarg, "r") == NULL)
			{
                                fprintf(stderr, "%s: Unable to open config file \"%s\".\n",
					program, optarg);
				exit(1);
			}
                        else
			{
				++config_flag;
				sprintf(tmp_opt, "-c %s ", optarg);
				strcat(general_opt, tmp_opt);
			}
                        break;
		case 'q' :
			++quiet_flag;
			strcat(general_opt, "-q ");
			break;
                case '?':
                        ++errflg;
			break;
		/*
		 * astmonitor options
		 */
                case 'm' :
                        ++monitor_flag;
			strcpy(monitor_opt, "-m ");
                        break;
		/*
		 * astdisplay options
		 */
		case 'u' :
			++util_flag;
			strcpy(display_opt, "-u ");
			break;
                case 't' :
                        ++text_flag;
			sprintf(tmp_opt, "-t \"%s\" ", optarg);
			strcat(display_opt, tmp_opt);
                        break;
		/*
		 * astgraph options
		 */
                case 'h' :
                        ++hist_flag;
			strcpy(graph_opt, "-h ");
                        break;
                case 's' :
                        ++status_flag;
			strcpy(graph_opt, "-s ");
                        break;
                case 'o' :
                        ++online_flag;
			strcpy(graph_opt, "-o ");
                        break;
		/*
		 * astcache options
		 */
                case 'n' :
                        ++enable_flag;
			strcpy(cache_opt, "-n ");
                        break;
                case 'f' :
                        ++disable_flag;
			strcpy(cache_opt, "-f ");
                        break;
                }
        }

	/* check incompatible options and arguments */
	if (util_flag && text_flag)
	{
		fprintf(stderr, "%s: Select only one display mode\n",
			program);
		++errflg;
	}

	if (hist_flag + status_flag + online_flag > 1)
	{
		fprintf(stderr, "%s: Select only one bar graph mode\n",
			program);
		++errflg;
	}

	if (enable_flag && disable_flag)
	{
		fprintf(stderr,
			"%s: -n and -f options are mutually exclusive\n",
			program);
		++errflg;
	}

	/* check number of arguments */
        if (argc - optind > 0)
                ++errflg;

        if (errflg) {
                usage();
                exit(2);
        }

	if (enable_flag || disable_flag || config_flag)
		run_cache();
	if (monitor_flag || config_flag)
		run_monitor();
	if (hist_flag || status_flag || online_flag || config_flag)
		run_graph();
	if (util_flag || text_flag || config_flag)
		run_display();
	if (! (monitor_flag || util_flag || text_flag ||
	       hist_flag || status_flag || online_flag ||
	       enable_flag || disable_flag || config_flag))
		get_info();

        exit(0);
}

/*
 * usage ()
 * Give a concise message on how to use this program
 */
usage()
{
        fprintf(stderr, "Usage:\t%s [-m] [-u | -t \"text\" ] [-h | -s | -o] [-c file] [-q]\n", 
	   program);
	fprintf(stderr, 
"	-m         initiates front panel monitor\n\
	-u         shows CPU utilization on front panel display\n\
	-t \"text\"  shows text on front panel display\n\
	-h         shows CPU utilization on front panel CPU graph\n\
	-s         shows which CPUs are in use on front panel CPU graph\n\
	-o         shows which CPUs are on-line on front panel CPU graph\n\
	-q         quiet mode; only errors are reported\n\
	-c file    reads from configuration file\n\
  With no arguments, report on current state of front panel\n\
  monitors and displays.\n");
}

void checkuid()
{
	if (((int)getuid()) != 0)
	{
		fprintf(stderr, "%s: Must be root to use\n", program);
		exit(1);
	}
}

void run_monitor()
{
	char cmd_str[STRSIZE];

	sprintf(cmd_str, "astmonitor %s %s\n", general_opt, monitor_opt);
	system(cmd_str);
}

void run_display()
{
	char cmd_str[STRSIZE];

	sprintf(cmd_str, "astdisplay %s %s\n", general_opt, display_opt);
	system(cmd_str);
	
}

void run_graph()
{
	char cmd_str[STRSIZE];

	sprintf(cmd_str, "astgraph %s %s\n", general_opt, graph_opt);
	system(cmd_str);
}

void run_cache()
{
	char cmd_str[STRSIZE];

	sprintf(cmd_str, "astcache %s %s\n", general_opt, cache_opt);
	system(cmd_str);
}

void get_info()
{
	char cmd_str[STRSIZE];

	sprintf(cmd_str, "astmonitor %s %s\n", general_opt, monitor_opt);
	system(cmd_str);

	sprintf(cmd_str, "astdisplay %s %s\n", general_opt, display_opt);
	system(cmd_str);

	sprintf(cmd_str, "astgraph %s %s\n", general_opt, graph_opt);
	system(cmd_str);

}


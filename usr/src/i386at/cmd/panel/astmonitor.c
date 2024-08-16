/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:astmonitor.c	1.5"

/*
 * astmonitor.c - monitors several aspects of the AST Manhattan MP
 *	Four functions
 *		1)  Monitors front panel buttons (OFF and ATTN)
 *		2)  Monitors power source (UPS state)
 *		3)  Monitors thermal condition
 *		4)  Monitors power supply condition
 */

#define TRUE	1
#define FALSE	0

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>		/* string manipulations */

#include <sys/types.h>		/* primitive system data types */
#include <sys/stat.h>		/* stat data types */
#include <fcntl.h>		/* file control */
#include <errno.h>		/* error handling */
#include <sys/errno.h>		/* error handling */
#include <sys/ebi.h>		/* defines the AST EBI II interface */
#include <time.h>		/* include files for time libraries */
#include <signal.h>		/* handles signals, like SIGCHLD */
#include <unistd.h>		/* header file for symbolic constants */

#define	STRSIZE	80

void open_ioctl_dev();
void interrupt_daemon();
void cleanup_ilockfile();
void attn_interrupt();
void shut_interrupt();
void ups_interrupt();
void therm_daemon();
void cleanup_tlockfile();
void pwr_daemon();
void cleanup_plockfile();
void get_info();
void reset_waiting_flag();
void rootmail();
void waitncheck_therm();
void read_config_file();
void checkuid();

int	fd;				/* file descriptor */
char	device[] = "/dev/astm";		/* device */
char	program[] = "astmonitor";	/* pointer to the program name */
char	cmd_str[STRSIZE];		/* string to build commands to pass to system */
char	err_str[STRSIZE];		/* string to build errors to pass to perror */
char	ilockfile[] = "/etc/panel/astmoni.lock";
char	tlockfile[] = "/etc/panel/astmont.lock";
char	plockfile[] = "/etc/panel/astmonp.lock";

int	monit_flag = 0; 	/* have the monitoring daemon been selected? */
int	ups_action_flag=0;	/* power-fail action has been specified */
int	quiet_flag = 0;		/* are we in quiet mode? */
int	config_flag = 0;	/* has a configuration file been supplied? */
FILE	*config_file;		/* file pointer for config file */
FILE	*flag_file;		/* file pointer for flag files */

int	waiting_flag = FALSE;	/* are we running a waiting process? */

int	ups_shut_delay = 1;	/* number of minutes between pwr_fail and shutdown */
int	therm_shut_delay = 1;	/* number of minutes between pwr_fail and shutdown */
int	therm_poll_delay = 1;	/* number of minutes between polling */
int	pwr_poll_delay = 1;	/* number of minutes between polling */

char	ups_command[STRSIZE] = "cd /;/sbin/shutdown -y -g0 -i0";

#define UPS_SHUT_DELAY		0 
#define UPS_ACTION 		1
#define THERM_SHUT_DELAY	2
#define THERM_POLL_DELAY	3
#define PWR_POLL_DELAY		4

#define MAX_TOKENS		5

char *KEYWORD_TBL[] =
{
"UPS_SHUT_DELAY",
"UPS_ACTION",
"THERM_SHUT_DELAY",
"THERM_POLL_DELAY",
"PWR_POLL_DELAY"
};

char tr1[] = "abcdefghijklmnopqrstuvwxyz-";
char tr2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

void main(argc,argv)
int argc;
char *argv[];
{
        int c, errflg = 0;
	char options_string[] = "mqc:?";
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
				exit(1);
			}
                        else
			{
				++config_flag;
				++monit_flag;
			}
                        break;
		case 'm' :
			/* set flags */
			++monit_flag;
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

        if (errflg) {
                giveusage();
                exit(2);
        }

	if (config_flag)
		read_config_file();

        /* open the ioctl device to do ioctl calls */
        open_ioctl_dev();

	if (! (monit_flag || config_flag))
		get_info();

	if (monit_flag)
	{
		interrupt_daemon();
		pwr_daemon();
		therm_daemon();
	}

	exit(0);
}

/*
 * Giveusage ()
 * Give a concise message on how to use this program
 */
giveusage()
{
        fprintf(stderr,"Usage: %s [-m] [-c configfile] [-q]\n", program);
        fprintf(stderr,
"	-m       initiates monitoring daemons:\n\
			monitors ATTN button and OFF button\n\
			monitors power source\n\
			monitors thermal sensor\n\
			monitors power supply condition\n\
	-c file  read from configuration file\n\
	-q       quiet mode; only errors are reported\n\
  With no arguments, reports the status of the astmonitor daemons.\n"); }

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
		case UPS_SHUT_DELAY:
			ups_shut_delay = atoi(value);
			break;
		case UPS_ACTION:
			strcpy(ups_command, value);
			++ups_action_flag;		
			break;
		case THERM_SHUT_DELAY:
			therm_shut_delay = atoi(value);
			break;
		case THERM_POLL_DELAY:
			therm_poll_delay = atoi(value);
			break;
		case PWR_POLL_DELAY:
			pwr_poll_delay = atoi(value);
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
        if ((fd = open(device, O_RDONLY)) == -1)
        {
                fprintf(stderr, "%s: Cannot open %s device\n", program, device);
                fprintf(stderr, "%s: Unable to start monitoring daemons\n", program);
                exit(1);
        }
}

/*
 *********************  CHECK FOR LOCK DIRECTORY
 */

void chklockdir()
{
	struct stat buf;

	if (stat("/etc/panel", &buf) == -1 &&
	   (errno == ENOENT))
		system("mkdir /etc/panel");
}

/*
 *********************  INTERRUPT DAEMON
 */

void interrupt_daemon()
{
	int event_code = 0;
	int pid;			/* process id from flag file */
	FILE *lockfile;			/* file for reading lockfile */
	FILE *pipeout;			/* file for output of ps and grep */
	char out_str[STRSIZE];		/* string for results of pipe */

	chklockdir();

	if ((lockfile = fopen(ilockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			fprintf(stderr, "Already monitoring front panel buttons\n");
			fprintf(stderr, "Already monitoring power source\n");
			return;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if (! quiet_flag) 
	{
		fprintf(stderr, "Monitoring front panel buttons\n");
		fprintf(stderr, "Monitoring power source\n");
	}

	fflush(stderr);

	switch(fork()) {
	case 0:
		/* when process is killed, clean up */
		signal(SIGHUP, cleanup_ilockfile);
		signal(SIGINT, cleanup_ilockfile);
		signal(SIGQUIT, cleanup_ilockfile);
		signal(SIGTERM, cleanup_ilockfile);

		/* write flag file to /tmp */
		if ((flag_file = fopen(ilockfile, "w")) == NULL)
		{
			sprintf(err_str, "%s: %s", program, optarg);
			perror(err_str);
			exit(1);
		}
		fprintf(flag_file, "%d\n", (int)getpid());
		fclose(flag_file);

		/* clear event flags */
		ioctl(fd, EBI_CLEAR_EVENTS);

		/* endless loop: keep processing interrupts until the end of time */
		while (1)
		{
			/* call ioctl, which sleeps until interrupt */
			if ((event_code = ioctl(fd, EBI_GET_EVENT)) == -1)
			{
				perror(program);
				exit(1);
			}

			switch (event_code)
			{
			case EBI_EVENT_ATTENTION:	/* event_code == 2 */
				attn_interrupt();
				break;
			case EBI_EVENT_SHUTDOWN:	/* event_code == 1 */
				shut_interrupt();
				break;
			case EBI_EVENT_PWR_FAIL:	/* event_code == 0 */
				if (! waiting_flag) 
					ups_interrupt();
				break;
			}
		}
		/* this line never reached */
	}
}

void cleanup_ilockfile()
{
	sprintf(cmd_str, "rm -f %s", ilockfile);
	system(cmd_str);
	exit(99);
}

/*
 *********************  ATTN DAEMON
 */

void attn_interrupt()
{
	/* issue messages to console */
	fprintf(stderr, "%s: ATTENTION: ATTN button pressed -- Switching to single user mode\n", 
		program);
	/* issue message via mail to root */
	rootmail("ATTENTION", "ATTN button pressed -- Switching to single user mode\n");

	system("/sbin/init s");
	/* and now back to the old grind of monitoring interrupts */
}

/*
 *********************  SHUTDOWN DAEMON
 */

void shut_interrupt()
{
	/* issue messages to console */
	fprintf(stderr, "%s: SHUTDOWN: OFF selected -- System shutting down\n", program);
	/* issue message via mail to root */
	rootmail("SHUTDOWN", "OFF selected -- System shutting down\n");

#ifdef PWROFF_IOCTL
	/* call ioctl so that psm_reboot will TURN OFF THE POWER */
	if (ioctl(fd, EBI_SET_PWR_OFF) == -1)
	{
			fprintf(stderr, "%s: Unable to turn off power\n", 
				program);
			perror(program);
	}
#endif

	system("cd /;/sbin/shutdown -y -g0 -i0");
	/* should never get to the next line */
	exit(1);
}

/*
 *********************  UPS DAEMON
 */

void ups_interrupt()
{
	time_t time_value;
	int event_code = 0;
	char	message[STRSIZE];

	/* get the time so we can print it to the console */
	time_value = time((time_t *)0);

	/* issue messages to console */
	fprintf(stderr, "%s: POWER FAILURE:  Running on UPS power  %s", 
		program, ctime(&time_value));
	fprintf(stderr, "%s: POWER FAILURE:  Will execute the following command in %d minutes:\n", 
		program, ups_shut_delay);
	fprintf(stderr, "\t%s\n", ups_command);
	/* issue message via mail to root */
	sprintf(message, "Power lost -- System will soon execute command:\n\t%s", ups_command);
	rootmail("POWER FAILURE", message);

	/* if we have blinking power generating another interupt */
	/* we don't want to spawn another wait-and-execute process */
	/* so we'll set the waiting_flag to let us know we already have a */
	/* wait-and-execute process */
	waiting_flag = TRUE;

	signal(SIGCHLD, SIG_IGN);
	/* fork a new process to wait and then execute ups_command */
	/* we fork a new process because we want the parent program
	 * to go back and continue monitoring the other events */	 
        switch(fork()) {
        case 0:
		/* go to sleep until execute ups_command */
		sleep(ups_shut_delay * 60);

		/* message to console about ups_command */
		fprintf(stderr, "%s: POWER FAILURE:  Executing following command:\n", 
		program);
		fprintf(stderr, "\t%s\n", ups_command);
		sprintf(message, "Power lost -- System executing command:\n\t%s", ups_command);
		/* issue message via mail to root */
		rootmail("POWER FAILURE", message);

		/* if this is the default shutdown, */
#ifdef PWROFF_IOCTL
		/* call ioctl so that psm_reboot will TURN OFF THE POWER */
		if (! ups_action_flag)
			if (ioctl(fd, EBI_SET_PWR_OFF) == -1)
			{
				fprintf(stderr, "%s: Unable to turn off power\n", 
					program);
				perror(program);
			}
#endif

		system(ups_command);
		/* if the shutdown worked, program should not get to here */
		fprintf(stderr, "%s: POWER FAILURE:  Can't run shutdown!\n", 
			program);
		exit(1);

        }
	/* when child ends, reset the waiting_flag */
	signal(SIGCHLD, reset_waiting_flag);
	/* and now back to the old grind of monitoring interrupts */
}

void reset_waiting_flag()
{
	waiting_flag = FALSE;

	/*signal(SIGCHLD, SIG_DFL);*/
}

/*
 *********************  THERM DAEMON
 */

void therm_daemon()
{
	int thermal_state = 0;
	int pid;			/* process id from flag file */
	FILE *lockfile;			/* file for reading lockfile */
	FILE *pipeout;			/* file for output of ps and grep */
	char out_str[STRSIZE];		/* string for results of pipe */

	chklockdir();

	if ((lockfile = fopen(tlockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			fprintf(stderr, "Already monitoring thermal condition\n");
			return;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Monitoring thermal condition\n");

	fflush(stderr);

	switch(fork()) {
	case 0:
		/* when process is killed, clean up */
		signal(SIGHUP, cleanup_tlockfile);
		signal(SIGINT, cleanup_tlockfile);
		signal(SIGQUIT, cleanup_tlockfile);
		signal(SIGTERM, cleanup_tlockfile);

		/* write flag file to /tmp */
		if ((flag_file = fopen(tlockfile, "w")) == NULL)
		{
			sprintf(err_str, "%s: %s", program, optarg);
			perror(err_str);
			exit(1);
		}
		fprintf(flag_file, "%d\n", (int)getpid());
		fclose(flag_file);

		while (1)
		{
			/* get the thermal state */
			if (ioctl(fd, EBI_GET_THERMAL_STATE, &thermal_state) == -1)
			{
				perror(program);
				exit(1);
			}

			/* if we are hot to trot... */
			if (thermal_state)
			{
				waitncheck_therm();
			}
			/* if we are cool as a cucumber... */

			/* sleep until we poll again */
			sleep(therm_poll_delay * 60);
		}
		/* this line never reached */
	}
}

void cleanup_tlockfile()
{
	sprintf(cmd_str, "rm -f %s", tlockfile);
	system(cmd_str);
	exit(99);
}

void waitncheck_therm()
{
	time_t time_value;
	int thermal_state = 0;
	char	message[STRSIZE];

	/* get the time so we can print it to the console */
	time_value = time((time_t *)0);

	/* issue messages to console */
	fprintf(stderr, "%s: OVERHEATED:  Cooling system initiated %s", 
		program, ctime(&time_value));
	fprintf(stderr,
		"%s: OVERHEATED:  Shutdown in %d minutes if still hot\n", 
		program, therm_shut_delay);
	rootmail("OVERHEATED", "System overheated -- will shutdown if system continues to run hot");

	/* go to sleep until we'll check if system is still hot */
	sleep(therm_shut_delay * 60);

	/* get the thermal state */
	if (ioctl(fd, EBI_GET_THERMAL_STATE, &thermal_state) == -1)
	{
		perror(program);
		exit(1);
	}

	/* if we are _still_ hot to trot... */
	if (thermal_state)
	{
		/* message to console about shutdown */
		fprintf(stderr, "%s: OVERHEATED:  System shutting down\n", program);
		/* issue message via mail to root */
		rootmail("SHUTDOWN", "System overheated -- System shutting down");

#ifdef PWROFF_IOCTL
		/* call ioctl so that psm_reboot will TURN OFF THE POWER */
		if (ioctl(fd, EBI_SET_PWR_OFF) == -1)
		{
				fprintf(stderr, "%s: Unable to turn off power\n", 
					program);
				perror(program);
		}
#endif

		system("cd /;/sbin/shutdown -y -g0 -i0");
		/* if the exec worked, program should not get to here */
		fprintf(stderr, "%s: OVERHEATED:  Can't run shutdown!\n", 
			program);
		exit(1);
	}
	fprintf(stderr, "%s: NORMAL TEMPERATURE RESTORED:  Shutdown cancelled\n", program);
	rootmail("NORMAL TEMPERATURE", "Normal temperature restored -- shutdown cancelled");

	/* and now back to the old grind of polling the thermal condition */
}

/*
 *********************  PWR DAEMON
 */

void pwr_daemon()
{
	int number_ps, i;
	int present, onLine;
	int ps_fail_flag = 0;
	struct pwrinfo get_psi;
	powerSupplyInfo psi;
	int pid;			/* process id from flag file */
	FILE *lockfile;			/* file for reading lockfile */
	FILE *pipeout;			/* file for output of ps and grep */
	char out_str[STRSIZE];		/* string for results of pipe */

	chklockdir();

	if ((lockfile = fopen(plockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			fprintf(stderr, "Already monitoring power supply condition\n");
			return;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if (! quiet_flag) 
		fprintf(stderr,"Monitoring power supply condition\n");

	fflush(stderr);

	/* place psi pointer in get_psi struct */
	get_psi.ps_info = &psi;

	if (ioctl(fd, EBI_GET_NUM_PWR_SUPPLIES, &number_ps) == -1)
	{
		perror(program);
		exit(1);
	}

	switch(fork()) {
	case 0:
		/* when process is killed, clean up */
		signal(SIGHUP, cleanup_plockfile);
		signal(SIGINT, cleanup_plockfile);
		signal(SIGQUIT, cleanup_plockfile);
		signal(SIGTERM, cleanup_plockfile);

		/* write flag file to /tmp */
		if ((flag_file = fopen(plockfile, "w")) == NULL)
		{
			sprintf(err_str, "%s: %s", program, optarg);
			perror(err_str);
			exit(1);
		}
		fprintf(flag_file, "%d\n", (int)getpid());
		fclose(flag_file);

		while (1)
		{
			for (i=0; i < number_ps; i++)
			{
				/* tell the ioctl which power supply */
				get_psi.ps_num = i;
				/* get the power supply info */
				if (ioctl(fd, EBI_GET_PWR_INFO, &get_psi) == -1)
				{
					perror(program);
					exit(1);
				}
				if (get_psi.ps_info->present &&
				   (! get_psi.ps_info->onLine))
					++ps_fail_flag;
			}
			if (ps_fail_flag)
			{
				/* message to console about failure */
				fprintf(stderr, 
				  "%s: POWER SUPPLY FAILURE:  System lost one or more power supplies.\n", 
				  program);
				/* issue message via mail to root */
				rootmail("POWER SUPPLY FAILURE", 
				"Power supply failure - You have lost one or more power supplies");
			}

			/* sleep until we poll again */
			sleep(pwr_poll_delay * 60);
		}
		/* this line never reached */
	}
}

void cleanup_plockfile()
{
	sprintf(cmd_str, "rm -f %s", plockfile);
	system(cmd_str);
	exit(99);
}

void rootmail(char *subject_str, char *message_str)
{
	/* prepare command for system */
	sprintf(cmd_str, "/bin/echo \'%s\' | /usr/bin/mailx -s \'%s\' root", 
		message_str, subject_str);
	system(cmd_str);
}


void get_info()
{
	int	iflag, tflag, pflag;
	int pid;			/* process id from flag file */
	FILE *lockfile;			/* file for reading lockfile */
	FILE *pipeout;			/* file for output of ps and grep */
	char out_str[STRSIZE];		/* string for results of pipe */

	iflag = tflag = pflag = 0;

	if ((lockfile = fopen(ilockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			++iflag;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if ((lockfile = fopen(plockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			++pflag;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if ((lockfile = fopen(tlockfile, "r")) != NULL)
	{
		/* 
		 * scan lockfile for pid of monitor process
		 */
		if (fscanf(lockfile,"%d", &pid) != 1)
		{
			/* it fails if the lockfile is bad */
			/* do something */
		}
		/* 
		 * look for pid in process table
		 */
		sprintf(cmd_str, "ps -e | fgrep %d | fgrep astmon", pid);
		/* 
		 * open resulting pipe
		 */
		pipeout = popen(cmd_str, "r");
		/* 
		 * scan for pid from output of pipe
		 */
		if (fscanf(pipeout, "%s\n", &out_str) == 1)
		{
			/* 
			 * it succeeds if this pid shows up in
			 * process table -- we are already running
			 * this monitoring process
			 */
			++tflag;
		}
		/* then close pipe */
		pclose(pipeout);
	}

	if (! (iflag || tflag || pflag))
		printf("Front panel monitor is not running\n", program);
	else
	{
		printf("Front panel monitor is monitoring:\n", program);

		if (iflag)
		{
			printf("\tfront panel buttons\n");
			printf("\tpower source\n");
		}
		if (pflag)
		{
			printf("\tpower supply condition\n");
		}
		if (tflag)
		{
			printf("\tthermal condition\n");
		}
	}
}



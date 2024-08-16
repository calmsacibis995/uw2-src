/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/mailproc.c	1.1.1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mailproc.c	1.3 'attmail mail(1) command'"
/* from ferret SCCSid ferret.c 3.1 */
#include "../mail/libmail.h"
#include "mailproc.h"

FILE	*Mailfp;			/* mail file pointer */
Msg	*Msgptr = NULL;			/* current message */
int	aflg = 0;			/* process all messages */
int	dflg = 0;			/* debug flag */
int	lflg = 0;			/* list only */
int	nflg = 0;			/* no action flag */
int	oflg = 0;			/* printf stdout of cmd */
int	pflg = 0;			/* read 1 msg from stdin */
int	rflg = 0;			/* read-only */
int	sflg = 0;			/* read stdin */
int	myegid;				/* the effective group id */
int	myrgid;				/* the real group id */

static char tempfile[MAXPATHLEN];	/* tmp file name */
static char MAILPROCRC[] = "MAILPROCRC";		/* mailproc rc env var */
static char MAILPROCDIR[] = "MAILPROCDIR";		/* mailproc save dir */
static char CMDFILE[] = ".mailproc_rc";			/* $HOME/.mailproc_rc */

static	void	rm_tmp ARGS((int));		/* remove tmp file, unlock, and exit */
static	void	maildashF ARGS((int, char**));	/* install mail Forwarding information */

#define PRIV(x)	(setgid(myegid), (_priv = (int)(x)), setgid(myrgid), _priv)
static int _priv;

/* print a usage message and leave */
void usage(argv)
char **argv;
{
	(void) pfmt(stderr, MM_ACTION,
	    ":567:Usage: %s [-alnoprsvV] [-e expr] [-f mailfile] [-c cmdfile] [-d save]\n", argv[0]);
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	char		altcmd[MAXPATHLEN];	/* alternate command path */
	char		altmpath[MAXPATHLEN];	/* alternate mail path */
	int		c;			/* getopt() character */
	Cmd		*cmd = 0;		/* command line cmd expr */
	char		*cmdfile = 0;		/* command file */
	char		cwd[MAXPATHLEN];	/* current working dir */
	int		eflg = 0;		/* -e found? */
	int		errflg = 0;		/* error flag */
	char		*homedir = 0;		/* home directory */
	int		found_new = 0;		/* found new messages? */
	Cmd		*last = 0;		/* last command */
	char		*logname = 0;		/* login name */
	long		mailend = 0;		/* end of mail file */
	char		maildirbuf[MAXPATHLEN];	/* copy of mailfile */
	char		*maildir = 0;		/* directory half of mailfile */
	char		*mailfile = 0;		/* mailfile */
	FILE		*mailfp = 0;		/* mail file pointer */
	char		*mailname = 0;		/* name half of mailfile */
	Msg		*mp = 0;		/* message list pointer */
	int		newmsgs = 0;		/* new messages left? */
	int		ret;			/* return value */
	char		*savedir = 0;		/* dir to save msgs in */
	struct stat	statbuf;		/* status buffer */
	FILE		*tmpfp = 0;		/* tmp file pointer */

	/* Initialize the privileges, locale and message database */
	(void) setlocale(LC_ALL, "");
	(void) setcat("uxemail");
	(void) setlabel("UX:mailproc");

#ifdef YYDEBUG					/* turn on yacc tracing */
	if (getenv("MAILPROC_YYDEBUG"))
		yydebug++;
#endif

	while((c = getopt(argc, argv, "ac:d:e:f:lnoprsvVy")) != EOF)
	{
		switch(c)
		{
			case 'a':		/* include old msgs also */
				aflg++;
				break;
			case 'c':		/* Command File */
				cmdfile = optarg;
				break;
			case 'd':		/* save directory */
				savedir = optarg;
				break;
			case 'e':		/* cmd expr */
				/* allocate an entry */
				if ((cmd = new_Cmd(optarg)) == NULL)
				{
					(void) pfmt(stderr, MM_ERROR, ":382:Problem allocating memory\n");
					exit(1);
				}

				(void) strcat(cmd->cmdbuf, "\n");
				if (!Cmdlist)
					Cmdlist = cmd;
				else
					last->next = cmd;
				last = cmd;
				eflg++;
				break;
			case 'f':		/* Mail File */
				mailfile = optarg;
				break;
			case 'l':		/* list messages */
				lflg++;
				break;
			case 'n':		/* no action */
				nflg++;
				dflg++;
				break;
			case 'o':		/* print command stdout */
				oflg++;
				break;
			case 'r':		/* readonly */
				rflg++;
				break;
			case 'p':		/* read input as 1 msg */
				pflg++;
				/* FALLTHROUGH */
			case 's':		/* use std streams */
				sflg++;
				break;
			case 'v':		/* verbose output */
				dflg++;
				break;
			case 'V':		/* Version */
				(void) fprintf(stdout, "mailproc 1.3\n");
				exit(0);
				/* NOTREACHED */
			case 'y':		/* install mail -F */
				maildashF(argc, argv);
				/* NOTREACHED */
			case '?':
				errflg++;
				break;
		}
	}

	if (errflg || optind < argc)
		usage(argv);

	/* map upper case to lower case for regex matching */
	for (c = 0; c < 256; c++)
		if (isupper(c)) re_map[c] = tolower(c);
		else re_map[c] = (char)c;

	homedir = getenv("HOME");
	if ((homedir == NULL) || (*homedir == '\0'))
	{
		(void) pfmt(stderr, MM_ERROR, ":568:no %s environment variable specified\n", "HOME");
		exit(1);
	}

	/* get current working directory */
	if (!getcwd(cwd, sizeof(cwd)))
	{
		(void) pfmt(stderr, MM_ERROR, ":569:cannot get current working directory\n");
		exit(1);
	}

	myegid = getegid();
	myrgid = getgid();
	setgid(myrgid);

	if (!mailfile && !sflg)
	{
		/* no arguments, process the mail file given by MAIL */
		mailfile = getenv("MAIL");
		if ((mailfile == NULL) || (*mailfile == '\0'))
		{
			/* mail path not found -- try /usr/mail/LOGNAME */
			logname = getenv("LOGNAME");
			if ((logname) && (*logname))
			{
				(void) strcpy(altmpath, MAILDIR);
				(void) strcat(altmpath, logname);
				mailfile = altmpath;
			}
			else
			{
				/* no mail file found */
				(void) pfmt(stderr, MM_ERROR, ":570:Cannot determine name of mailfile\n");
				exit(1);
			}
		}
	}
	else if (!sflg && (*mailfile != '/'))
	{
		/* convert to absolute path name */
		(void) sprintf(altmpath, "%s/%s", cwd, mailfile);
		mailfile = altmpath;
	}

	/* if lflg (list only) is set, parse mail now, display and exit */
	if (lflg)
	{
		if (sflg)
			mailfp = stdin;
		else if ((mailfp = fopen(mailfile, "r")) == NULL)
		{
			(void) pfmt(stderr, MM_ERROR, ":403:Cannot open mailfile: %s\n", Strerror(errno));
			exit(2);
		}

		ret = parsemail(mailfp, NULL);
		exit(ret >= 0 ? ret : 255);
	}

	if (!sflg)
	{
		strcpy(maildirbuf, mailfile);
		maildir = maildirbuf;
		mailname = strrchr(maildirbuf, '/');
		if (mailname)
			*mailname++ = '\0';
		else
			mailname = maildirbuf;
		if (!maildir[0])
			maildir = ".";

	}

	if (!cmdfile && !eflg)
	{
		cmdfile = getenv(MAILPROCRC);
		if ((cmdfile == NULL) || (*cmdfile == '\0'))
		{
			(void) sprintf(altcmd, "%s/%s", homedir, CMDFILE);
			cmdfile = altcmd;
		}
	}
	if ( cmdfile && (*cmdfile != '/'))
	{
		(void) sprintf(altcmd, "%s/%s", cwd, cmdfile);
		cmdfile = altcmd;
	}

	if (!savedir)
	{
		savedir = getenv(MAILPROCDIR);
		if ((savedir == NULL) || (*savedir == '\0'))
			savedir = homedir;
	}

	/* change directory to save directory */
	if (chdir(savedir) < 0)
		(void) pfmt(stderr, MM_ERROR, ":9:Cannot change directory to \"%s\": %s\n", savedir, Strerror(errno));
	else if (dflg)
		(void)pfmt(stderr, MM_INFO, ":571:** Changing directory to %s\n", savedir);

	/* parse command file */
	if (cmdfile)
	{
		if (!cmdparse(cmdfile))
		{
			(void) fclose(tmpfp);
			exit(0);
		}
	}

	/* open tmp file */
	(void)umask(077);
	/* create tmp file to hold mailbox */
	(void) tmpnam(tempfile);
	if ((tmpfp = fopen(tempfile, "w+")) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n", tempfile, Strerror(errno));
		exit(1);
	}

	/* catch signals */
	(void) signal(SIGHUP, rm_tmp);
	(void) signal(SIGQUIT, rm_tmp);
	(void) signal(SIGINT, rm_tmp);

	if (dflg)
	{
		/* print out flags values */
		(void) pfmt(stderr, MM_INFO, ":572:** Debug mode is on\n");
		if (!aflg)
			(void) pfmt(stderr, MM_INFO, ":573:** Processing new messages only\n");
		else
			(void) pfmt(stderr, MM_INFO, ":574:** Processing all messages\n");
		(void) pfmt(stderr, MM_INFO, ":575:** Reading mail from %s\n", sflg ? "stdin" : mailfile);
	}

	/* lock the file */
	if (!sflg && !nflg && !rflg)
		PRIV(maildlock(mailname, 10, maildir, 1));

	/* Beginning of processing ... */

	/* open mail file */
	if (sflg)
		mailfp = stdin;
	else if ((mailfp = fopen(mailfile, "r")) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n", mailfile, Strerror(errno));
		PRIV(mailunlock());
		exit(2);
	}

	/* parse mail file */
	found_new = parsemail(mailfp, tmpfp);

	/* if nothing to process, just die */
	if (!found_new && !sflg)
	{
		if (dflg)
			(void) pfmt(stderr, MM_INFO, ":576:no messages to process\n");

		(void) fclose(mailfp);
		(void) fclose(tmpfp);
		(void) unlink(tempfile);
		PRIV(mailunlock());
		exit(0);
	}

	/* reopen tempfile for reading */
	rewind(tmpfp);

	/* Do we have anything to process? */
	if (found_new)
	{
		/* process the messages */
		Mailfp = tmpfp;
		for(mp = msglist; mp; mp = mp->next)
		{
			if (!process(mp))
			{
				(void) unlink(tempfile);
				PRIV(mailunlock());
				exit(1);
			}
		}

		if (!sflg)
			(void) fclose(mailfp);
	}

	/* if in no action mode, don't bother copying back to the mail file */
	if (nflg || rflg)
	{
		(void) unlink(tempfile);
		if (nflg || dflg)
			(void) pfmt(stderr, MM_INFO, ":577:Exiting from %s\n", argv[0]);
		exit(0);
	}

	/* critical section of code -- catch signals */
	if (!sflg)
	{
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
	}

	/* copy back the file */
	rewind(tmpfp);

	/* use :saved! ???? */
	if (sflg)
		mailfp = stdout;
	else if ((mailfp = fopen(mailfile, "w")) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, ":578:Cannot copy mail file back: %s\n", Strerror(errno));
		(void) unlink(tempfile);
		PRIV(mailunlock());
		exit(1);
	}

	(void) copyback(fileno(tmpfp), fileno(mailfp), msglist);
	(void) unlink(tempfile);
	PRIV(mailunlock());

	if (dflg)
		(void) pfmt(stderr, MM_INFO, ":577:Exiting from %s\n", argv[0]);

	if (pflg)
		return msglist[0].keep ? 1 : 0;
	return 0;
}

/* rm_tmp will remove tmp file after signal */
static void rm_tmp(sig)
int sig;
{
	(void) signal(sig, rm_tmp);
	(void) unlink(tempfile);
	PRIV(mailunlock());
	exit(sig);
}

/* install a "Forward to" personal mail surrogate */
static void maildashF(argc, argv)
int argc;
char **argv;
{
	static char cmdstr[1024] = "mail -F '| S=0;C=1;F=*; mailproc -p ";
	char *bufp = cmdstr + strlen(cmdstr);
	register int c;

	optind = 1;			/* restart getopt's scan */
	Quoted = '\'';			/* initialize quotechars()'s scan */
	while((c = getopt(argc, argv, "ac:d:e:f:lnoprsvVy")) != EOF)
		switch(c)
		{
			/* pass these options along */
			case 'c':		/* Command File */
			case 'd':		/* save directory */
			case 'e':		/* cmd expr */
			case 'f':		/* Mail File */
				sprintf(bufp, "-%c '%s' ", c, quotechars(optarg));
				bufp += strlen(bufp);
				break;

			/* ignore the -y option this time around */
			case 'y': break;

			/* no other options can be used with -y */
			case 'a': case 'l': case 'n': case 'o': case 'p':
			case 'r': case 's': case 'v': case 'V':
			case '?':
				usage(argv);
				/* NOTREACHED */
		}

	strcat(bufp, "'");

	if (optind < argc)
		usage(argv);

	exit(system(cmdstr));
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init.c	1.19.5.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)init.c	2.59 'attmail mail(1) command'"
 /*
  * All global externs defined in mail.h. All variables are initialized
  * here!
  *
  * !!!!!IF YOU CHANGE (OR ADD) IT HERE, DO IT THERE ALSO !!!!!!!!
  *
  */
#include	"mail.h"
/*#define	endgrent	abi_endgrent*/

const char	Binary[] = "binary";
const char	GenericText[] = "generic-text";
const char	Text[] = "text";
char		dbgfname[20];
FILE		*dbgfp;
int		debug;	/* Controls debugging level. 0 ==> no debugging */
const char	*errlist[] = {	/* Words to go along with E_* error numbers */
		"",
  /*E_REMOTE*/	":367:Unknown system\n",
  /*E_FILE*/	":368:Problem with mailfile\n",
  /*E_SPACE*/	":369:Space problem\n",
  /*E_FRWD*/	":370:Unable to forward mail, check permissions and group\n",
  /*E_SYNTAX*/	":490:Syntax error\n",
  /*E_FRWL*/	":371:Forwarding loop\n",
  /*E_SNDR*/	":372:Invalid sender\n",
  /*E_USER*/	":373:Invalid recipient\n",
  /*E_FROM*/	":374:Too many From lines\n",
  /*E_PERM*/	":375:Invalid permissions\n",
  /*E_MBOX*/	":376:Cannot open mbox\n",
  /*E_TMP*/	":377:Temporary file problem\n",
  /*E_DEAD*/	":378:Cannot create dead.letter\n",
  /*E_UNBND*/	":379:Unbounded forwarding\n",
  /*E_LOCK*/	":380:Cannot create lock file\n",
  /*E_GROUP*/	":381:No group id of 'mail'\n",
  /*E_MEM*/	":382:Problem allocating memory\n",
  /*E_FORK*/	":383:Could not fork\n",
  /*E_PIPE*/	":384:Cannot pipe\n",
  /*E_OWNR*/	":385:Must be owner to modify mailfile\n",
  /*E_DENY*/	":386:Permission denied by /etc/mail/mailsurr file\n",
  /*E_SURG*/	":345:Surrogate command failed\n",
  /*E_NBNRY*/	":393:Cannot send binary to remote\n",
  /*E_TRAN*/	":342:Translation command failed\n"
};
int		error = 0;	/* Local value for error */
char		*failsafe;	/* $FAILSAFE */
int		flgd = 0;	/*  1 ==> 'd' test option -- show work in progress */
int		flge = 0;	/*  1 ==> 'e'      option -- check existence of mail */
char		*flgf = 0;	/* !0 ==> 'f' read option -- mbox to use, holds value */
char		*ReturnPath = NULL; /* !NULL ==> 'f' rmail option -- from address */
char		*flgF = 0;	/* !0 ==> 'F'      option -- Installing/Removing Forwarding, holds value */
int		flgh = 0;	/*  1 ==> 'h' read option -- start mail printing with header list */
char		*flgm = 0;	/* !0 ==> 'm' send option -- holds message type */
int		flgp = 0;	/*  1 ==> 'p' read option -- no prompts when reading mail */
int		flgP = 0;	/*  1 ==> 'P' read option -- always print binary mail */
int		flgq = 0;	/*  1 ==> 'q' read option -- interrupts don't exit while reading mail */
int		flgr = 0;	/*  1 ==> 'r' read option -- print in fifo order */
int		flgt = 0;	/*  1 ==> 't' send option -- add To: line to letter */
int		flgT = 0;	/*  1 ==> 'T' test option -- surrogate file to debug */
int		flgw = 0;	/*  1 ==> 'w' send option -- don't wait on delivery programs */
int		flgx = 0;	/* !0 ==> 'x' test option -- dump debug messages */
int		flglb = 0;	/*  1 ==> '#'      option -- show command to be run */
/* Default_display indicates whether to display this header line to the TTY */
/* when in default mode. Can be overridden via 'P' command at ? prompt */
const Hdr	header[H_MAX] = {
		{ "",					FALSE,  0 },
		{ "From ",				TRUE,   5 },	/* H_FROM */
		{ "From ", /* ... remote from ... */	TRUE,   5 },	/* H_RFROM */
		{ ">From ",				TRUE,   6 },	/* H_FROM1 */
		{ "Return-path:",			FALSE, 12 },	/* H_RETURN_PATH */
		{ "Received:",				FALSE,  9 },	/* H_RECEIVED */
		{ "Message-Version:",			FALSE, 16 },	/* H_MVERS */
		{ "Report-Version:",			FALSE, 15 },	/* H_RVERS */
		{ ">To:",				FALSE,  4 },	/* H_TCOPY */
		{ "Date:",				TRUE,   5 },	/* H_DATE */
		{ "Original-Date:",			TRUE,  14 },	/* H_ODATE */
		{ "From:",				TRUE,   5 },	/* H_FROM2 */
		{ "Auto-Forwarded-From:",		TRUE,  20 },	/* H_AFWDFROM */
		{ "Original-Auto-Forwarded-From:",	TRUE,  29 },	/* H_OAFWDFROM */
		{ "Auto-Forward-Count:",		FALSE, 19 },	/* H_AFWDCNT */
		{ "MTS-Message-ID:",			FALSE, 15 },	/* H_MTSID */
		{ "Confirming-MTS-Message-ID:",		TRUE,  26 },	/* H_CMTSID */
		{ "UA-Content-ID:",			FALSE, 14 },	/* H_UAID */
		{ "Confirming-UA-Content-ID:",		TRUE,  25 },	/* H_CUAID */
		{ "Delivered-To:",			TRUE,  13 },	/* H_DLVRTO */
		{ "Not-Delivered-To:",			TRUE,  17 },	/* H_NDLVRTO */
		{ "En-Route-To:",			TRUE,  12 },	/* H_ENROUTE */
		{ "End-of-Header:",			FALSE, 14 },	/* H_EOH */
		{ "EMail-Version:",			FALSE, 14 },	/* H_EVERS */
		{ "Phone:",				FALSE,  6 },	/* H_PHONE */
		{ "Fax-Phone:",				FALSE, 10 },	/* H_FAXPHONE */
		{ "Subject:",				TRUE,   8 },	/* H_SUBJ */
		{ "Original-Subject:",			TRUE,  17 },	/* H_OSUBJ */
		{ "UA-Message-ID:",			FALSE, 14 },	/* H_UAMID */
		{ "To:",				TRUE,   3 },	/* H_TO */
		{ "Reply-To:",				TRUE,   9 },	/* H_REPLYTO */
		{ "Cc:",				TRUE,   3 },	/* H_CC */
		{ "Bcc:",				TRUE,   4 },	/* H_BCC */
		{ "End-of-Protocol:",			FALSE, 16 },	/* H_EOP */
		{ "Message-Type:",			FALSE, 13 },	/* H_MTYPE */
		{ "Message-Service:",			TRUE,  16 },	/* H_MSVC */
		{ "Original-Message-Service:",		TRUE,  25 },	/* H_OMSVC */
		{ "Default-Options:",			FALSE, 16 },	/* H_DEFOPTS */
		{ "Transport-Options:",			FALSE, 18 },	/* H_TROPTS */
		{ "Errors-To:",				FALSE, 10 },	/* H_ERRTO */
		{ "Content-Length:",			TRUE,  15 },	/* H_CLEN */
		{ "Content-Type:",			FALSE, 13 },	/* H_CTYPE */
		{ "Encoding-Type:",			TRUE,  14 },	/* H_ENCDTYPE */
		{ "Sender:",				FALSE,  7 },	/* H_SENDER */
		{ "Precedence:",			FALSE, 11 },	/* H_PRECEDENCE */
		{ "Mime-Version:",			FALSE, 13 },	/* H_MIME_VERSION */
		{ "Message-ID:",			FALSE, 13 },	/* H_MESSAGE_ID */
		{ "Content-Transfer-Encoding:",		FALSE, 13 },	/* H_CTE */
		{ "Illegal-To:",			FALSE, 13 },	/* H_ILL_TO */
		{ "Illegal-Cc:",			FALSE, 13 },	/* H_ILL_CC */
		{ "Illegal-Bcc:",			FALSE, 13 },	/* H_ILL_BCC */
		{ "Illegal-From:",			FALSE, 13 },	/* H_ILL_FROM2 */
		{ "Illegal-Sender:",			FALSE, 13 },	/* H_ILL_SENDER */
		{ "Illegal-Reply-To:",			FALSE, 13 },	/* H_ILL_REPLYTO */
		{ "Illegal-Errors-To:",			FALSE, 13 },	/* H_ILL_ERRTO */
		{ "Name-Value:",			FALSE, 11 },	/* H_NAMEVALUE */
		{ "",					FALSE,  0 },	/* H_CONT */
};
const char	*help[] = {	/* help messages during printmail() */
		":540:?,*\t\tprint this help message\n",
		":86:#\t\tdisplay message number #\n",
		":87:-\t\tprint previous\n",
		":88:+\t\tnext (no delete)\n",
		":89:! cmd\t\texecute cmd\n",
		":90:<CR>\t\tnext (no delete)\n",
		":91:a\t\tposition at and read newly arrived mail\n",
		":92:d [#]\t\tdelete message # (default current message)\n",
		":93:dp\t\tdelete current message and print the next\n",
		":94:dq\t\tdelete current message and exit\n",
		":95:h a\t\tdisplay all headers\n",
		":96:h d\t\tdisplay headers of letters scheduled for deletion\n",
		":97:h [#]\t\tdisplay headers around # (default current message)\n",
		":541:m [user] \tmail (and delete) current message to user (default yourself)\n",
		":542:M [user] \tmail (and delete) current message to user, with comments\n",
		":99:n\t\tnext (no delete)\n",
		":100:p\t\tprint (override any warnings of binary content)\n",
		":101:P\t\toverride default 'brief' mode and display ALL header lines\n",
		":102:q, ^D\t\tquit\n",
		":103:r [args]\treply to (and delete) current letter via mail [args]\n",
		":469:R [args]\treply to (and delete) current letter via mail [args], including message\n",
		":104:s [files]\tsave (and delete) current message (default mbox)\n",
		":105:u [#]\t\tundelete message # (default current message)\n",
		":106:w [files]\tsave (and delete) current message without header\n",
		":107:x\t\texit without changing mail\n",
		":108:y [files]\tsave (and delete) current message (default mbox)\n",
		0
};
char		*hmbox;			/* pointer to $HOME/mbox */
char		*home;			/* pointer to $HOME */
int		interrupted = 0;	/* some signal came in */
int		ismail = TRUE;		/* default to program=mail */
char		*mailfile;		/* pointer to mailfile */
gid_t		mailgrp;		/* numeric id of group 'mail' */
char		*mailsurr = MAILSURR;	/* surrogate file name */
int		maxerr = 0;		/* largest value of error */
const char	mbox[] = "/mbox";	/* name for mbox */
uid_t		mf_uid;			/* uid of users mailfile */
gid_t		mf_gid;			/* gid of users mailfile */
char		my_name[20];		/* user's name who invoked this command */
uid_t		my_euid;		/* user's euid */
gid_t		my_egid;		/* user's egid */
uid_t		my_uid;			/* user's uid */
gid_t		my_gid;			/* user's gid */
const char	*nometamail;		/* getenv("NOMETAMAIL") */
const char	*progname;		/* program name */
char		*remotefrom;		/* Holds name of the system to use in "remote from" */
const char *rewritefile = "/etc/mail/rewrite";/* name of file instead of /etc/mail/rewrite */
int		sav_errno;		/* errno from writing to files */
void		(*saveint)();		/* saved signal from SIG_INT */
/* Any header line prefixes listed here WILL be displayed in default mode */
/* If it's not here, it won't be shown. Can be overridden via 'P' command */
/* at ? prompt */
const Hdr	seldisp[] = {		/* selective display header field prefixes */
		{ "Cc:",		FALSE, 3 },
		{ "Bcc:",		FALSE, 4 },
		{ "Paper-",		FALSE, 6 },
		{ "Phone:",		FALSE, 6 },
		{ "Message-",		FALSE, 8 },
		{ "Original-",		FALSE, 9 },
		{ "Confirming-",	FALSE, 11 },
		{ "Delivered-",		FALSE, 10 },
		{ "Deliverable-",	FALSE, 12 },
		{ "Not-",		FALSE, 4 },
		{ "En-Route-To:",	FALSE, 12 },
		{ 0, 0, 0 }
};
jmp_buf		sjbuf;		/* Where to longjmp for signals */
int		surr_len = 0;	/* # entries in surrogate file */
char		*thissys;	/* Holds name of the system we are on */
char		tmpdir[] = "";	/* default directory for tmp files */
Msg		*topmsg;	/* Current message being worked on */
Tmpfile		*toptmpfile;	/* List of the temp files being used. */
mode_t		umsave;		/* saved umask */

void init()
{
    static const char pn[] = "init";
    struct group *grpptr;	/* holds /etc/group entry for group mail */
    struct passwd *pwd;		/* holds /etc/passwd entry for this user */

    /*
	Get group id for mail, exit if none exists
    */
    if ((grpptr = getgrnam("mail")) == NULL) {
	errmsg(E_GROUP,"");
	exit(1);
    } else {
	mailgrp = grpptr->gr_gid;
    }
    endgrent();

    /*
	Save the *id for later use.
    */
    my_uid = getuid();
    my_gid = getgid();
    my_euid = geteuid();
    my_egid = getegid();

    /* What command (rmail or mail)? */
    if (strcmp(basename(progname), "rmail") == SAME)
	ismail = FALSE;

    if (debug == 0)
	{
	/* If not set as an invocation option, check for system-wide */
	/* global flag */
	char *xp = mgetenv("DEBUG");
	if (xp != (char *)NULL)
	    {
	    debug = flgx = atoi(xp);
	    if (debug < 0)
		debug = -debug; /* Keep trace file even if successful */
	    }
	}

    if (debug > 0)
	{
	strcpy (dbgfname, "/tmp/MLDBGXXXXXX");
	mktemp (dbgfname);
	if ((dbgfp = fopen(dbgfname,"w")) == (FILE *)NULL)
	    {
	    pfmt(stderr, MM_ERROR,
		":2:Cannot open %s: %s\n",
		dbgfname, Strerror(errno));
	    exit (13);
	    }
	setbuf (dbgfp, NULL);
	fprintf(dbgfp, "main(): debugging level == %d\n", debug);
	fprintf(dbgfp, "main(): trace file ='%s': kept %s\n", dbgfname,
	    ((flgx < 0) ?
		"on success or failure." : "only on failure."));
	}

    umsave = umask(MAILMASK);
    thissys = mailsystem(0);
    remotefrom = mgetenv("REMOTEFROM");
    if (!remotefrom)
	remotefrom = thissys;
    Dout(pn, 11, "thissys = '%s', uname = '%s', remotefrom = '%s'\n",
	thissys, mailsystem(1), remotefrom);

    failsafe = mgetenv("FAILSAFE");
    if (failsafe)
	Dout(pn, 11, "failsafe processing enabled to %s\n", failsafe);

    /* Retrieve $HOME environment variable */
    home = getenv("HOME");
    if (!home || !*home)
	home = ".";
    Dout(pn, 11, "$HOME set to %s\n", home);

    /* Determine login name. */
    strcpy(my_name, "Unknown");
    pwd = getpwuid(my_euid);
    if (pwd)
	strncpy (my_name, pwd->pw_name, sizeof(my_name));
    Dout(pn, 11, "my_name = '%s'\n", my_name);

    nometamail = getenv("NOMETAMAIL");
}

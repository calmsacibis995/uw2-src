/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/telnet/commands.c	1.1.1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)commands.c	5.4 (Berkeley) 3/1/91";
#endif /* not lint */

#if	defined(unix)
#include <sys/param.h>
#include <sys/types.h>
#ifdef	CRAY
#include <sys/types.h>
#endif
#include <sys/file.h>
#else
#endif	/* defined(unix) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef	CRAY
#include <fcntl.h>
#endif	/* CRAY */

#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include <pwd.h>
#include <varargs.h>
#include <errno.h>
#include <pfmt.h>

#include <arpa/telnet.h>

#include "general.h"

#include "ring.h"

#include "externs.h"
#include "defines.h"
#include "types.h"

#ifndef CRAY
#include <netinet/in_systm.h>
# if (defined(vax) || defined(tahoe) || defined(hp300)) && !defined(ultrix)
# include <machine/endian.h>
# endif /* vax */
#endif /* CRAY */
#include <netinet/ip.h>

#if !defined(M_UNIX) && !defined(M_XENIX)
extern char *strerror();
#endif

#ifndef       MAXHOSTNAMELEN
#define       MAXHOSTNAMELEN 64
#endif     /* MAXHOSTNAMELEN */

#if	defined(IPPROTO_IP) && defined(IP_TOS)
int tos = -1;
#endif	/* defined(IPPROTO_IP) && defined(IP_TOS) */

#define bzero(b, n)	memset(b, '\0', n)
#define index(s, c)	strchr(s, c)
#define rindex(s, c)	strrchr(s, c)

char	*hostname;
static char _hostname[MAXHOSTNAMELEN];

extern char *getenv();

extern int isprefix();
extern char **genget();
extern int Ambiguous();

static call();

typedef struct {
	char	*name;		/* command name */
	char	*help;		/* help string (NULL for no help) */
	int	(*handler)();	/* routine which executes command */
	int	needconnect;	/* Do we need to be connected to execute? */
} Command;

static char line[256];
static char saveline[256];
static int margc;
static char *margv[20];

#if defined(u3b2)
extern struct passwd *getpwnam();
extern struct passwd *getpwuid();
#endif

extern void send_will(), send_wont(), send_do(), send_dont();

    static void
makeargv()
{
    register char *cp, *cp2, c;
    register char **argp = margv;

    margc = 0;
    cp = line;
    if (*cp == '!') {		/* Special case shell escape */
	strcpy(saveline, line);	/* save for shell command */
	*argp++ = "!";		/* No room in string to get this */
	margc++;
	cp++;
    }
    while (c = *cp) {
	register int inquote = 0;
	while (isspace(c))
	    c = *++cp;
	if (c == '\0')
	    break;
	*argp++ = cp;
	margc += 1;
	for (cp2 = cp; c != '\0'; c = *++cp) {
	    if (inquote) {
		if (c == inquote) {
		    inquote = 0;
		    continue;
		}
	    } else {
		if (c == '\\') {
		    if ((c = *++cp) == '\0')
			break;
		} else if (c == '"') {
		    inquote = '"';
		    continue;
		} else if (c == '\'') {
		    inquote = '\'';
		    continue;
		} else if (isspace(c))
		    break;
	    }
	    *cp2++ = c;
	}
	*cp2 = '\0';
	if (c == '\0')
	    break;
	cp++;
    }
    *argp++ = 0;
}

/*
 * Make a character string into a number.
 *
 * Todo:  1.  Could take random integers (12, 0x12, 012, 0b1).
 */

	static
special(s)
	register char *s;
{
	register char c;
	char b;

	switch (*s) {
	case '^':
		b = *++s;
		if (b == '?') {
		    c = b | 0x40;		/* DEL */
		} else {
		    c = b & 0x1f;
		}
		break;
	default:
		c = *s;
		break;
	}
	return c;
}

/*
 * Construct a control character sequence
 * for a special character.
 */
	static char *
control(c)
	register cc_t c;
{
	static char buf[5];
	/*
	 * The only way I could get the Sun 3.5 compiler
	 * to shut up about
	 *	if ((unsigned int)c >= 0x80)
	 * was to assign "c" to an unsigned int variable...
	 * Arggg....
	 */
	register unsigned int uic = (unsigned int)c;

	if (uic == 0x7f)
		return ("^?");
	if (c == (cc_t)_POSIX_VDISABLE) {
		return "off";
	}
	if (uic >= 0x80) {
		buf[0] = '\\';
		buf[1] = ((c>>6)&07) + '0';
		buf[2] = ((c>>3)&07) + '0';
		buf[3] = (c&07) + '0';
		buf[4] = 0;
	} else if (uic >= 0x20) {
		buf[0] = c;
		buf[1] = 0;
	} else {
		buf[0] = '^';
		buf[1] = '@'+c;
		buf[2] = 0;
	}
	return (buf);
}



/*
 *	The following are data structures and routines for
 *	the "send" command.
 *
 */
 
struct sendlist {
    char	*name;		/* How user refers to it (case independent) */
    char	*help;		/* Help information (0 ==> no help) */
    int		needconnect;	/* Need to be connected */
    int		narg;		/* Number of arguments */
    int		(*handler)();	/* Routine to perform (for special ops) */
    int		nbyte;		/* Number of bytes to send this command */
    int		what;		/* Character to be sent (<0 ==> special) */
};


extern int
	send_esc P((void)),
	send_help P((void)),
	send_docmd P((char *)),
	send_dontcmd P((char *)),
	send_willcmd P((char *)),
	send_wontcmd P((char *));

static struct sendlist Sendlist[] = {
    { "ao",	":7:Send Telnet Abort output",		1, 0, 0, 2, AO },
    { "ayt",	":8:Send Telnet 'Are You There'",		1, 0, 0, 2, AYT },
    { "brk",	":9:Send Telnet Break",			1, 0, 0, 2, BREAK },
    { "break",	0,					1, 0, 0, 2, BREAK },
    { "ec",	":10:Send Telnet Erase Character",		1, 0, 0, 2, EC },
    { "el",	":11:Send Telnet Erase Line",		1, 0, 0, 2, EL },
    { "escape",	":12:Send current escape character",	1, 0, send_esc, 1, 0 },
    { "ga",	":13:Send Telnet 'Go Ahead' sequence",	1, 0, 0, 2, GA },
    { "ip",	":14:Send Telnet Interrupt Process",	1, 0, 0, 2, IP },
    { "intp",	0,					1, 0, 0, 2, IP },
    { "interrupt", 0,					1, 0, 0, 2, IP },
    { "intr",	0,					1, 0, 0, 2, IP },
    { "nop",	":15:Send Telnet 'No operation'",		1, 0, 0, 2, NOP },
    { "eor",	":16:Send Telnet 'End of Record'",		1, 0, 0, 2, EOR },
    { "abort",	":17:Send Telnet 'Abort Process'",		1, 0, 0, 2, ABORT },
    { "susp",	":18:Send Telnet 'Suspend Process'",	1, 0, 0, 2, SUSP },
    { "eof",	":19:Send Telnet End of File Character",	1, 0, 0, 2, xEOF },
    { "synch",	":20:Perform Telnet 'Synch operation'",	1, 0, dosynch, 2, 0 },
    { "getstatus", ":21:Send request for STATUS",		1, 0, get_status, 6, 0 },
    { "?",	":22:Display send options",			0, 0, send_help, 0, 0 },
    { "help",	0,					0, 0, send_help, 0, 0 },
    { "do",	0,					0, 1, send_docmd, 3, 0 },
    { "dont",	0,					0, 1, send_dontcmd, 3, 0 },
    { "will",	0,					0, 1, send_willcmd, 3, 0 },
    { "wont",	0,					0, 1, send_wontcmd, 3, 0 },
    { 0 }
};

#define	GETSEND(name) ((struct sendlist *) genget(name, (char **) Sendlist, \
				sizeof(struct sendlist)))

    static int
sendcmd(argc, argv)
    int  argc;
    char **argv;
{
    int what;		/* what we are sending this time */
    int count;		/* how many bytes we are going to need to send */
    int i;
    int question = 0;	/* was at least one argument a question */
    struct sendlist *s;	/* pointer to current command */
    int success = 0;
    int needconnect = 0;

    if (argc < 2) {
	pfmt(stdout, MM_NOSTD, ":23:need at least one argument for 'send' command\n");
	pfmt(stdout, MM_NOSTD, ":24:'send ?' for help\n");
	return 0;
    }
    /*
     * First, validate all the send arguments.
     * In addition, we see how much space we are going to need, and
     * whether or not we will be doing a "SYNCH" operation (which
     * flushes the network queue).
     */
    count = 0;
    for (i = 1; i < argc; i++) {
	s = GETSEND(argv[i]);
	if (s == 0) {
	    pfmt(stdout, MM_NOSTD, ":25:Unknown send argument '%s'\n'send ?' for help.\n",
			argv[i]);
	    return 0;
	} else if (Ambiguous(s)) {
	    pfmt(stdout, MM_NOSTD, ":26:Ambiguous send argument '%s'\n'send ?' for help.\n",
			argv[i]);
	    return 0;
	}
	if (i + s->narg >= argc) {
	    if (s->narg == 1)
		pfmt(stderr, MM_ERROR,
			":27:Need 1 argument to 'send %s' command.  'send %s ?' for help.\n",
			s->name, s->name);
	    else
		pfmt(stderr, MM_ERROR,
			":28:Need %d arguments to 'send %s' command.  'send %s ?' for help.\n",
			s->narg, s->name, s->name);
	    return 0;
	}
	count += s->nbyte;
	if (s->handler == send_help) {
	    send_help();
	    return 0;
	}

	i += s->narg;
	needconnect += s->needconnect;
    }
    if (!connected && needconnect) {
	pfmt(stdout, MM_NOSTD, ":29:?Need to be connected first.\n");
	pfmt(stdout, MM_NOSTD, ":24:'send ?' for help\n");
	return 0;
    }
    /* Now, do we have enough room? */
    if (NETROOM() < count) {
	pfmt(stdout, MM_NOSTD, ":30:There is not enough room in the buffer TO the network\nto process your request.  Nothing will be done.\n('send synch' will throw away most data in the network\nbuffer, if this might help.)\n");
	return 0;
    }
    /* OK, they are all OK, now go through again and actually send */
    count = 0;
    for (i = 1; i < argc; i++) {
	if ((s = GETSEND(argv[i])) == 0) {
	    pfmt(stderr, MM_ERROR, ":34:Telnet 'send' error - argument disappeared!\n");
	    (void) quit();
	    /*NOTREACHED*/
	}
	if (s->handler) {
	    count++;
	    success += (*s->handler)((s->narg > 0) ? argv[i+1] : 0,
				  (s->narg > 1) ? argv[i+2] : 0);
	    i += s->narg;
	} else {
            what = s->what;
	    NET2ADD(IAC, what);
	    printoption("SENT", IAC, what);
	}
    }
    return (count == success);
}

    static int
send_esc()
{
    NETADD(escape);
    return 1;
}

    static int
send_docmd(name)
    char *name;
{
    return(send_tncmd(send_do, "do", name));
}

    static int
send_dontcmd(name)
    char *name;
{
    return(send_tncmd(send_dont, "dont", name));
}
    static int
send_willcmd(name)
    char *name;
{
    return(send_tncmd(send_will, "will", name));
}
    static int
send_wontcmd(name)
    char *name;
{
    return(send_tncmd(send_wont, "wont", name));
}

    int
send_tncmd(func, cmd, name)
    void	(*func)();
    char	*cmd, *name;
{
    char **cpp;
    extern char *telopts[];

    if (isprefix(name, "help") || isprefix(name, "?")) {
	register int col, len;

	pfmt(stdout, MM_NOSTD, ":35:Usage: send %s <option>\n", cmd);
	pfmt(stdout, MM_NOSTD, ":36:Valid options are:\n\t");

	col = 8;
	for (cpp = telopts; *cpp; cpp++) {
	    len = strlen(*cpp) + 1;
	    if (col + len > 65) {
		pfmt(stdout, MM_NOSTD, ":37:\n\t");
		col = 8;
	    }
	    pfmt(stdout, MM_NOSTD, ":38: %s", *cpp);
	    col += len;
	}
	pfmt(stdout, MM_NOSTD, ":39:\n");
	return 0;
    }
    cpp = (char **)genget(name, telopts, sizeof(char *));
    if (Ambiguous(cpp)) {
	pfmt(stderr, MM_ERROR, ":40:'%s': ambiguous argument ('send %s ?' for help).\n",
					name, cmd);
	return 0;
    }
    if (cpp == 0) {
	pfmt(stderr, MM_ERROR, ":41:'%s': unknown argument ('send %s ?' for help).\n",
					name, cmd);
	return 0;
    }
    if (!connected) {
	pfmt(stdout, MM_NOSTD, ":29:?Need to be connected first.\n");
	return 0;
    }
    (*func)(cpp - telopts, 1);
    return 1;
}

    static int
send_help()
{
    struct sendlist *s;	/* pointer to current command */
    for (s = Sendlist; s->name; s++) {
	if (s->help) {
	    pfmt(stdout, MM_NOSTD, ":42:%-15s ", s->name);
	    if (s->help && *s->help)
		pfmt(stdout, MM_NOSTD, s->help);
	    pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
    return(0);
}

/*
 * The following are the routines and data structures referred
 * to by the arguments to the "toggle" command.
 */

    static int
lclchars()
{
    donelclchars = 1;
    return 1;
}

    static int
togdebug()
{
#ifndef	NOT43
    if (net > 0 &&
	(SetSockOpt(net, SOL_SOCKET, SO_DEBUG, debug)) < 0) {
	    pfmt(stderr, MM_ERROR, ":43:setsockopt (SO_DEBUG): %s\n", strerror(errno));
    }
#else	/* NOT43 */
    if (debug) {
	if (net > 0 && SetSockOpt(net, SOL_SOCKET, SO_DEBUG, 0, 0) < 0)
	    pfmt(stderr, MM_ERROR, ":43:setsockopt (SO_DEBUG): %s\n", strerror(errno));
    } else
	pfmt(stdout, MM_NOSTD, ":44:Cannot turn off socket debugging\n");
#endif	/* NOT43 */
    return 1;
}


    static int
togcrlf()
{
    if (crlf) {
	pfmt(stdout, MM_NOSTD, ":45:Will send carriage returns as telnet <CR><LF>.\n");
    } else {
	pfmt(stdout, MM_NOSTD, ":46:Will send carriage returns as telnet <CR><NUL>.\n");
    }
    return 1;
}

int binmode;

    static int
togbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val >= 0) {
	binmode = val;
    } else {
	if (my_want_state_is_will(TELOPT_BINARY) &&
				my_want_state_is_do(TELOPT_BINARY)) {
	    binmode = 1;
	} else if (my_want_state_is_wont(TELOPT_BINARY) &&
				my_want_state_is_dont(TELOPT_BINARY)) {
	    binmode = 0;
	}
	val = binmode ? 0 : 1;
    }

    if (val == 1) {
	if (my_want_state_is_will(TELOPT_BINARY) &&
					my_want_state_is_do(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":47:Already operating in binary mode with remote host.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":48:Negotiating binary mode with remote host.\n");
	    tel_enter_binary(3);
	}
    } else {
	if (my_want_state_is_wont(TELOPT_BINARY) &&
					my_want_state_is_dont(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":49:Already in network ascii mode with remote host.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":50:Negotiating network ascii mode with remote host.\n");
	    tel_leave_binary(3);
	}
    }
    return 1;
}

    static int
togrbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val == -1)
	val = my_want_state_is_do(TELOPT_BINARY) ? 0 : 1;

    if (val == 1) {
	if (my_want_state_is_do(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":51:Already receiving in binary mode.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":52:Negotiating binary mode on input.\n");
	    tel_enter_binary(1);
	}
    } else {
	if (my_want_state_is_dont(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":53:Already receiving in network ascii mode.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":54:Negotiating network ascii mode on input.\n");
	    tel_leave_binary(1);
	}
    }
    return 1;
}

    static int
togxbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val == -1)
	val = my_want_state_is_will(TELOPT_BINARY) ? 0 : 1;

    if (val == 1) {
	if (my_want_state_is_will(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":55:Already transmitting in binary mode.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":56:Negotiating binary mode on output.\n");
	    tel_enter_binary(2);
	}
    } else {
	if (my_want_state_is_wont(TELOPT_BINARY)) {
	    pfmt(stdout, MM_NOSTD, ":57:Already transmitting in network ascii mode.\n");
	} else {
	    pfmt(stdout, MM_NOSTD, ":58:Negotiating network ascii mode on output.\n");
	    tel_leave_binary(2);
	}
    }
    return 1;
}


extern int togglehelp P((void));
#if	defined(AUTHENTICATE)
extern int auth_togdebug P((int));
#endif
#if	defined(ENCRYPT)
extern int EncryptAutoEnc P((int));
extern int EncryptAutoDec P((int));
extern int EncryptDebug P((int));
extern int EncryptVerbose P((int));
#endif

struct togglelist {
    char	*name;		/* name of toggle */
    char	*t_help;	/* toggle help message */
    char	*e_help;	/* enable help message */
    char	*d_help;	/* disable help message */
    int		(*handler)();	/* routine to do actual setting */
    int		*variable;
    char	*will_actionexplanation;
    char	*wont_actionexplanation;
};

static struct togglelist Togglelist[] = {
    { "autoflush",
	":59:toggle flushing of output when sending interrupt characters",
	":60:enable flushing of output when sending interrupt characters",
	":61:disable flushing of output when sending interrupt characters",
	    0,
		&autoflush,
		    ":62:Will flush output when sending interrupt characters.",
		    ":63:Won't flush output when sending interrupt characters." },
    { "autosynch",
	":64:toggle automatic sending of interrupt characters in urgent mode",
	":65:enable automatic sending of interrupt characters in urgent mode",
	":66:disable automatic sending of interrupt characters in urgent mode",
	    0,
		&autosynch,
		    ":67:Will send interrupt characters in urgent mode.",
		    ":68:Won't send interrupt characters in urgent mode." },
#if	defined(AUTHENTICATE)
    { "autologin",
	":69:toggle automatic sending of login and/or authentication info",
	":70:enable automatic sending of login and/or authentication info",
	":71:disable automatic sending of login and/or authentication info",
	    0,
		&autologin,
		    ":72:Will send login name and/or authentication information.",
		    ":73:Won't send login name and/or authentication information." },
    { "authdebug",
	":74:toggle Toggle authentication debugging",
	":75:enable Toggle authentication debugging",
	":76:disable Toggle authentication debugging",
	    auth_togdebug,
		0,
		     ":77:Will print authentication debugging information.",
		     ":78:Won't print authentication debugging information." },
#endif
#if	defined(ENCRYPT)
    { "autoencrypt",
	":79:toggle automatic encryption of data stream",
	":80:enable automatic encryption of data stream",
	":81:disable automatic encryption of data stream",
	    EncryptAutoEnc,
		0,
		    ":82:Will automatically encrypt output.",
		    ":83:Won't automatically encrypt output." },
    { "autodecrypt",
	":84:toggle automatic decryption of data stream",
	":85:enable automatic decryption of data stream",
	":86:disable automatic decryption of data stream",
	    EncryptAutoDec,
		0,
		    ":87:Will automatically decrypt input.",
		    ":88:Won't automatically decrypt input." },
    { "verbose_encrypt",
	":89:toggle Toggle verbose encryption output",
	":90:enable Toggle verbose encryption output",
	":91:disable Toggle verbose encryption output",
	    EncryptVerbose,
		0,
		    ":92:Will print verbose encryption output.",
		    ":93:Won't print verbose encryption output." },
    { "encdebug",
	":94:toggle Toggle encryption debugging",
	":95:enable Toggle encryption debugging",
	":96:disable Toggle encryption debugging",
	    EncryptDebug,
		0,
		    ":97:Will print encryption debugging information.",
		    ":98:Won't print encryption debugging information." },
#endif
    { "skiprc",
	":99:toggle don't read ~/.telnetrc file",
	":100:enable don't read ~/.telnetrc file",
	":101:disable don't read ~/.telnetrc file",
	    0,
		&skiprc,
		    ":102:Will read ~/.telnetrc file.",
		    ":103:Won't read ~/.telnetrc file." },
    { "binary",
	":104:toggle sending and receiving of binary data",
	":105:enable sending and receiving of binary data",
	":106:disable sending and receiving of binary data",
	    togbinary,
		0,
		    0 },
    { "inbinary",
	":107:toggle receiving of binary data",
	":108:enable receiving of binary data",
	":109:disable receiving of binary data",
	    togrbinary,
		0,
		    0 },
    { "outbinary",
	":110:toggle sending of binary data",
	":111:enable sending of binary data",
	":112:disable sending of binary data",
	    togxbinary,
		0,
		    0 },
    { "crlf",
	":113:toggle sending carriage returns as telnet <CR><LF>",
	":114:enable sending carriage returns as telnet <CR><LF>",
	":115:disable sending carriage returns as telnet <CR><LF>",
	    togcrlf,
		&crlf,
		    0 },
    { "crmod",
	":116:toggle mapping of received carriage returns",
	":117:enable mapping of received carriage returns",
	":118:disable mapping of received carriage returns",
	    0,
		&crmod,
		    ":119:Will map carriage return on output.",
		    ":120:Won't map carriage return on output." },
    { "localchars",
	":121:toggle local recognition of certain control characters",
	":122:enable local recognition of certain control characters",
	":123:disable local recognition of certain control characters",
	    lclchars,
		&localchars,
		    ":124:Will recognize certain control characters.",
		    ":125:Won't recognize certain control characters." },
    { " ", "", 0 },		/* empty line */
#if	defined(unix) && defined(TN3270)
    { "apitrace",
	":126:toggle (debugging) toggle tracing of API transactions",
	":127:enable (debugging) toggle tracing of API transactions",
	":128:disable (debugging) toggle tracing of API transactions",
	    0,
		&apitrace,
		    ":129:Will trace API transactions.",
		    ":130:Won't trace API transactions." },
    { "cursesdata",
	":131:toggle (debugging) toggle printing of hexadecimal curses data",
	":132:enable (debugging) toggle printing of hexadecimal curses data",
	":133:disable (debugging) toggle printing of hexadecimal curses data",
	    0,
		&cursesdata,
		    ":134:Will print hexadecimal representation of curses data.",
		    ":135:Won't print hexadecimal representation of curses data." },
#endif	/* defined(unix) && defined(TN3270) */
    { "debug",
	":136:toggle debugging",
	":137:enable debugging",
	":138:disable debugging",
	    togdebug,
		&debug,
		    ":139:Will turn on socket level debugging.",
		    ":140:Won't turn on socket level debugging." },
    { "netdata",
	":141:toggle printing of hexadecimal network data (debugging)",
	":142:enable printing of hexadecimal network data (debugging)",
	":143:disable printing of hexadecimal network data (debugging)",
	    0,
		&netdata,
		    ":144:Will print hexadecimal representation of network traffic.",
		    ":145:Won't print hexadecimal representation of network traffic." },
    { "prettydump",
	":146:toggle output of \"netdata\" to user readable format (debugging)",
	":147:enable output of \"netdata\" to user readable format (debugging)",
	":148:disable output of \"netdata\" to user readable format (debugging)",
	    0,
		&prettydump,
		    ":149:Will print user readable output for \"netdata\".",
		    ":150:Won't print user readable output for \"netdata\"." },
    { "options",
	":151:toggle viewing of options processing (debugging)",
	":152:enable viewing of options processing (debugging)",
	":153:disable viewing of options processing (debugging)",
	    0,
		&showoptions,
		    ":154:Will show option processing.",
		    ":155:Won't show option processing." },
#if	defined(unix)
    { "termdata",
	":156:toggle (debugging) toggle printing of hexadecimal terminal data",
	":157:enable (debugging) toggle printing of hexadecimal terminal data",
	":158:disable (debugging) toggle printing of hexadecimal terminal data",
	    0,
		&termdata,
		    ":159:Will print hexadecimal representation of terminal traffic.",
		    ":160:Won't print hexadecimal representation of terminal traffic." },
#endif	/* defined(unix) */
    { "?",
	0,
	0,
	0,
	    togglehelp },
    { "help",
	0,
	0,
	0,
	    togglehelp },
    { 0 }
};

    static int
togglehelp()
{
    struct togglelist *c;

    for (c = Togglelist; c->name; c++) {
	if (c->t_help) {
	    if (*c->t_help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (c->t_help && *c->t_help)
		    pfmt(stdout, MM_NOSTD, c->t_help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
    pfmt(stdout, MM_NOSTD, ":39:\n");
    pfmt(stdout, MM_NOSTD, ":161:%-15s display help information\n", "?");
    return 0;
}

    static void
settogglehelp(set)
    int set;
{
    struct togglelist *c;

    for (c = Togglelist; c->name; c++) {
	if (c->e_help) {
	    if (*c->e_help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (set) {
		    if (c->e_help && *c->e_help)
			pfmt(stdout, MM_NOSTD, c->e_help);
		}
		else {
		    if (c->d_help && *c->d_help)
			pfmt(stdout, MM_NOSTD, c->d_help);
		}
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
}

#define	GETTOGGLE(name) (struct togglelist *) \
		genget(name, (char **) Togglelist, sizeof(struct togglelist))

    static int
toggle(argc, argv)
    int  argc;
    char *argv[];
{
    int retval = 1;
    char *name;
    struct togglelist *c;

    if (argc < 2) {
	pfmt(stderr, MM_ERROR,
	    ":162:Need an argument to 'toggle' command.  'toggle ?' for help.\n");
	return 0;
    }
    argc--;
    argv++;
    while (argc--) {
	name = *argv++;
	c = GETTOGGLE(name);
	if (Ambiguous(c)) {
	    pfmt(stderr, MM_ERROR, ":163:'%s': ambiguous argument ('toggle ?' for help).\n",
					name);
	    return 0;
	} else if (c == 0) {
	    pfmt(stderr, MM_ERROR, ":164:'%s': unknown argument ('toggle ?' for help).\n",
					name);
	    return 0;
	} else {
	    if (c->variable) {
		*c->variable = !*c->variable;		/* invert it */
		if (c->will_actionexplanation && *c->variable)
		    pfmt(stdout, MM_NOSTD, c->will_actionexplanation);
		else if (c->wont_actionexplanation && !(*c->variable))
		    pfmt(stdout, MM_NOSTD, c->wont_actionexplanation);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    if (c->handler) {
		retval &= (*c->handler)(-1);
	    }
	}
    }
    return retval;
}

/*
 * The following perform the "set" command.
 */

#ifdef	USE_TERMIO
struct termio new_tc = { 0 };
#endif

struct setlist {
    char *name;				/* name */
    char *help;				/* help information */
    void (*handler)();
    cc_t *charp;			/* where it is located at */
};

static struct setlist Setlist[] = {
#ifdef	KLUDGELINEMODE
    { "echo", 	":165:character to toggle local echoing on/off", 0, &echoc },
#endif
    { "escape",	":166:character to escape back to telnet command mode", 0, &escape },
    { "rlogin", ":167:rlogin escape character", 0, &rlogin },
    { "tracefile", ":168:file to write trace information to", SetNetTrace, (cc_t *)NetTraceFile},
    { " ", "" },
    { " ", ":169:The following need 'localchars' to be toggled true", 0, 0 },
    { "flushoutput", ":170:character to cause an Abort Output", 0, termFlushCharp },
    { "interrupt", ":171:character to cause an Interrupt Process", 0, termIntCharp },
    { "quit",	":172:character to cause an Abort process", 0, termQuitCharp },
    { "eof",	":173:character to cause an EOF ", 0, termEofCharp },
    { " ", "" },
    { " ", ":174:The following are for local editing in linemode", 0, 0 },
    { "erase",	":175:character to use to erase a character", 0, termEraseCharp },
    { "kill",	":176:character to use to erase a line", 0, termKillCharp },
    { "lnext",	":177:character to use for literal next", 0, termLiteralNextCharp },
    { "susp",	":178:character to cause a Suspend Process", 0, termSuspCharp },
    { "reprint", ":179:character to use for line reprint", 0, termRprntCharp },
    { "worderase", ":180:character to use to erase a word", 0, termWerasCharp },
    { "start",	":181:character to use for XON", 0, termStartCharp },
    { "stop",	":182:character to use for XOFF", 0, termStopCharp },
    { "forw1",	":183:alternate end of line character", 0, termForw1Charp },
    { "forw2",	":183:alternate end of line character", 0, termForw2Charp },
    { "ayt",	":184:alternate AYT character", 0, termAytCharp },
    { 0 }
};

#if	defined(CRAY) && !defined(__STDC__)
/* Work around compiler bug in pcc 4.1.5 */
    void
_setlist_init()
{
#ifndef	KLUDGELINEMODE
#define	N 5
#else
#define	N 6
#endif
	Setlist[N+0].charp = &termFlushChar;
	Setlist[N+1].charp = &termIntChar;
	Setlist[N+2].charp = &termQuitChar;
	Setlist[N+3].charp = &termEofChar;
	Setlist[N+6].charp = &termEraseChar;
	Setlist[N+7].charp = &termKillChar;
	Setlist[N+8].charp = &termLiteralNextChar;
	Setlist[N+9].charp = &termSuspChar;
	Setlist[N+10].charp = &termRprntChar;
	Setlist[N+11].charp = &termWerasChar;
	Setlist[N+12].charp = &termStartChar;
	Setlist[N+13].charp = &termStopChar;
	Setlist[N+14].charp = &termForw1Char;
	Setlist[N+15].charp = &termForw2Char;
	Setlist[N+16].charp = &termAytChar;
#undef	N
}
#endif	/* defined(CRAY) && !defined(__STDC__) */

    static struct setlist *
getset(name)
    char *name;
{
    return (struct setlist *)
		genget(name, (char **) Setlist, sizeof(struct setlist));
}

    void
set_escape_char(s)
    char *s;
{
	if (rlogin != _POSIX_VDISABLE) {
		rlogin = (s && *s) ? special(s) : _POSIX_VDISABLE;
		pfmt(stdout, MM_NOSTD, ":185:Telnet rlogin escape character is '%s'.\n",
					control(rlogin));
	} else {
		escape = (s && *s) ? special(s) : _POSIX_VDISABLE;
		pfmt(stdout, MM_NOSTD, ":186:Telnet escape character is '%s'.\n", control(escape));
	}
}

    static int
setcmd(argc, argv)
    int  argc;
    char *argv[];
{
    int value;
    struct setlist *ct;
    struct togglelist *c;

    if (argc < 2 || argc > 3) {
	pfmt(stdout, MM_NOSTD, ":187:Format is 'set Name Value'\n'set ?' for help.\n");
	return 0;
    }
    if ((argc == 2) && (isprefix(argv[1], "?") || isprefix(argv[1], "help"))) {
	for (ct = Setlist; ct->name; ct++) {
	    pfmt(stdout, MM_NOSTD, ":42:%-15s ", ct->name);
	    if (ct->help && *ct->help)
		pfmt(stdout, MM_NOSTD, ct->help);
	    pfmt(stdout, MM_NOSTD, ":39:\n");
	}
	pfmt(stdout, MM_NOSTD, ":39:\n");
	settogglehelp(1);
	pfmt(stdout, MM_NOSTD, ":161:%-15s display help information\n", "?");
	return 0;
    }

    ct = getset(argv[1]);
    if (ct == 0) {
	c = GETTOGGLE(argv[1]);
	if (c == 0) {
	    pfmt(stderr, MM_ERROR, ":188:'%s': unknown argument ('set ?' for help).\n",
			argv[1]);
	    return 0;
	} else if (Ambiguous(c)) {
	    pfmt(stderr, MM_ERROR, ":189:'%s': ambiguous argument ('set ?' for help).\n",
			argv[1]);
	    return 0;
	}
	if (c->variable) {
	    if ((argc == 2) || (strcmp("on", argv[2]) == 0))
		*c->variable = 1;
	    else if (strcmp("off", argv[2]) == 0)
		*c->variable = 0;
	    else {
		pfmt(stdout, MM_NOSTD, ":190:Format is 'set togglename [on|off]'\n'set ?' for help.\n");
		return 0;
	    }
	    if (c->will_actionexplanation && *c->variable)
		pfmt(stdout, MM_NOSTD, c->will_actionexplanation);
	    else if (c->wont_actionexplanation && !(*c->variable))
		pfmt(stdout, MM_NOSTD, c->wont_actionexplanation);
	    pfmt(stdout, MM_NOSTD, ":39:\n");
	}
	if (c->handler)
	    (*c->handler)(1);
    } else if (argc != 3) {
	pfmt(stdout, MM_NOSTD, ":187:Format is 'set Name Value'\n'set ?' for help.\n");
	return 0;
    } else if (Ambiguous(ct)) {
	pfmt(stderr, MM_ERROR, ":189:'%s': ambiguous argument ('set ?' for help).\n",
			argv[1]);
	return 0;
    } else if (ct->handler) {
	(*ct->handler)(argv[2]);
	pfmt(stdout, MM_NOSTD, ":191:%s set to \"%s\".\n", ct->name, (char *)ct->charp);
    } else {
	if (strcmp("off", argv[2])) {
	    value = special(argv[2]);
	} else {
	    value = _POSIX_VDISABLE;
	}
	*(ct->charp) = (cc_t)value;
	pfmt(stdout, MM_NOSTD, ":192:%s character is '%s'.\n", ct->name, control(*(ct->charp)));
    }
    slc_check();
    return 1;
}

    static int
unsetcmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct setlist *ct;
    struct togglelist *c;
    register char *name;

    if (argc < 2) {
	pfmt(stderr, MM_ERROR,
	    ":193:Need an argument to 'unset' command.  'unset ?' for help.\n");
	return 0;
    }
    if (isprefix(argv[1], "?") || isprefix(argv[1], "help")) {
	for (ct = Setlist; ct->name; ct++) {
	    pfmt(stdout, MM_NOSTD, ":42:%-15s ", ct->name);
	    if (ct->help && *ct->help)
		pfmt(stdout, MM_NOSTD, ct->help);
	    pfmt(stdout, MM_NOSTD, ":39:\n");
	}
	pfmt(stdout, MM_NOSTD, ":39:\n");
	settogglehelp(0);
	pfmt(stdout, MM_NOSTD, ":161:%-15s display help information\n", "?");
	return 0;
    }

    argc--;
    argv++;
    while (argc--) {
	name = *argv++;
	ct = getset(name);
	if (ct == 0) {
	    c = GETTOGGLE(name);
	    if (c == 0) {
		pfmt(stderr, MM_ERROR, ":194:'%s': unknown argument ('unset ?' for help).\n",
			name);
		return 0;
	    } else if (Ambiguous(c)) {
		pfmt(stderr, MM_ERROR, ":195:'%s': ambiguous argument ('unset ?' for help).\n",
			name);
		return 0;
	    }
	    if (c->variable) {
		*c->variable = 0;
		if (c->will_actionexplanation && *c->variable)
		    pfmt(stdout, MM_NOSTD, c->will_actionexplanation);
		else if (c->wont_actionexplanation && !(*c->variable))
		    pfmt(stdout, MM_NOSTD, c->wont_actionexplanation);
	        pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    if (c->handler)
		(*c->handler)(0);
	} else if (Ambiguous(ct)) {
		pfmt(stderr, MM_ERROR, ":195:'%s': ambiguous argument ('unset ?' for help).\n",
			name);
	    return 0;
	} else if (ct->handler) {
	    (*ct->handler)(0);
	    pfmt(stdout, MM_NOSTD, ":196:%s reset to \"%s\".\n", ct->name, (char *)ct->charp);
	} else {
	    *(ct->charp) = _POSIX_VDISABLE;
	    pfmt(stdout, MM_NOSTD, ":192:%s character is '%s'.\n", ct->name, control(*(ct->charp)));
	}
    }
    return 1;
}

/*
 * The following are the data structures and routines for the
 * 'mode' command.
 */
#ifdef	KLUDGELINEMODE
extern int kludgelinemode;

    static int
dokludgemode()
{
    kludgelinemode = 1;
    send_wont(TELOPT_LINEMODE, 1);
    send_dont(TELOPT_SGA, 1);
    send_dont(TELOPT_ECHO, 1);
}
#endif

    static int
dolinemode()
{
#ifdef	KLUDGELINEMODE
    if (kludgelinemode)
	send_dont(TELOPT_SGA, 1);
#endif
    send_will(TELOPT_LINEMODE, 1);
    send_dont(TELOPT_ECHO, 1);
    return 1;
}

    static int
docharmode()
{
#ifdef	KLUDGELINEMODE
    if (kludgelinemode)
	send_do(TELOPT_SGA, 1);
    else
#endif
    send_wont(TELOPT_LINEMODE, 1);
    send_do(TELOPT_ECHO, 1);
    return 1;
}

    static int
dolmmode(bit, on)
    int bit, on;
{
    unsigned char c;
    extern int linemode;

    if (my_want_state_is_wont(TELOPT_LINEMODE)) {
	pfmt(stdout, MM_NOSTD, ":197:?Need to have LINEMODE option enabled first.\n");
	pfmt(stdout, MM_NOSTD, ":198:'mode ?' for help.\n");
	return 0;
    }

    if (on)
	c = (linemode | bit);
    else
	c = (linemode & ~bit);
    lm_mode(&c, 1, 1);
    return 1;
}

    int
setmode(bit)
{
    return dolmmode(bit, 1);
}

    int
clearmode(bit)
{
    return dolmmode(bit, 0);
}

struct modelist {
	char	*name;		/* command name */
	char	*help;		/* help string */
	int	(*handler)();	/* routine which executes command */
	int	needconnect;	/* Do we need to be connected to execute? */
	int	arg1;
};

extern int modehelp();

static struct modelist ModeList[] = {
    { "character", ":199:Disable LINEMODE option",	docharmode, 1 },
#ifdef	KLUDGELINEMODE
    { "",	":200:(or disable obsolete line-by-line mode)", 0 },
#endif
    { "line",	":201:Enable LINEMODE option",	dolinemode, 1 },
#ifdef	KLUDGELINEMODE
    { "",	":202:(or enable obsolete line-by-line mode)", 0 },
#endif
    { "", "", 0 },
    { "",	":203:These require the LINEMODE option to be enabled", 0 },
    { "isig",	":204:Enable signal trapping",	setmode, 1, MODE_TRAPSIG },
    { "+isig",	0,				setmode, 1, MODE_TRAPSIG },
    { "-isig",	":205:Disable signal trapping",	clearmode, 1, MODE_TRAPSIG },
    { "edit",	":206:Enable character editing",	setmode, 1, MODE_EDIT },
    { "+edit",	0,				setmode, 1, MODE_EDIT },
    { "-edit",	":207:Disable character editing",	clearmode, 1, MODE_EDIT },
    { "softtabs", ":208:Enable tab expansion",	setmode, 1, MODE_SOFT_TAB },
    { "+softtabs", 0,				setmode, 1, MODE_SOFT_TAB },
    { "-softtabs", ":207:Disable character editing",	clearmode, 1, MODE_SOFT_TAB },
    { "litecho", ":209:Enable literal character echo", setmode, 1, MODE_LIT_ECHO },
    { "+litecho", 0,				setmode, 1, MODE_LIT_ECHO },
    { "-litecho", ":210:Disable literal character echo", clearmode, 1, MODE_LIT_ECHO },
    { "help",	0,				modehelp, 0 },
#ifdef	KLUDGELINEMODE
    { "kludgeline", 0,				dokludgemode, 1 },
#endif
    { "", "", 0 },
    { "?",	":211:Print help information",	modehelp, 0 },
    { 0 },
};


    int
modehelp()
{
    struct modelist *mt;

    pfmt(stdout, MM_NOSTD, ":212:format is:  'mode Mode', where 'Mode' is one of:\n\n");
    for (mt = ModeList; mt->name; mt++) {
	if (mt->help) {
	    if (*mt->help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", mt->name);
		if (mt->help && *mt->help)
		    pfmt(stdout, MM_NOSTD, mt->help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
    return 0;
}

#define	GETMODECMD(name) (struct modelist *) \
		genget(name, (char **) ModeList, sizeof(struct modelist))

    static int
modecmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct modelist *mt;

    if (argc != 2) {
	pfmt(stdout, MM_NOSTD, ":213:'mode' command requires an argument\n");
	pfmt(stdout, MM_NOSTD, ":198:'mode ?' for help.\n");
    } else if ((mt = GETMODECMD(argv[1])) == 0) {
	pfmt(stderr, MM_ERROR, ":214:Unknown mode '%s' ('mode ?' for help).\n", argv[1]);
    } else if (Ambiguous(mt)) {
	pfmt(stderr, MM_ERROR, ":215:Ambiguous mode '%s' ('mode ?' for help).\n", argv[1]);
    } else if (mt->needconnect && !connected) {
	pfmt(stdout, MM_NOSTD, ":29:?Need to be connected first.\n");
	pfmt(stdout, MM_NOSTD, ":198:'mode ?' for help.\n");
    } else if (mt->handler) {
	return (*mt->handler)(mt->arg1);
    }
    return 0;
}

/*
 * The following data structures and routines implement the
 * "display" command.
 */

    static int
display(argc, argv)
    int  argc;
    char *argv[];
{
    struct togglelist *tl;
    struct setlist *sl;

#define	dotog(tl)	if (tl->variable && tl->will_actionexplanation) { \
			    if (*tl->variable) { \
			        pfmt(stdout, MM_NOSTD, tl->will_actionexplanation); \
			    } else { \
			        pfmt(stdout, MM_NOSTD, tl->wont_actionexplanation); \
			    } \
			    pfmt(stdout, MM_NOSTD, ":39:\n"); \
			}

#define	doset(sl)   if (sl->name && *sl->name != ' ') { \
			if (sl->handler == 0) \
			    pfmt(stdout, MM_NOSTD, ":217:%-15s [%s]\n", sl->name, control(*sl->charp)); \
			else if (strcmp(sl->name, "tracefile") || *(char *)sl->charp) \
			    pfmt(stdout, MM_NOSTD, ":218:%-15s \"%s\"\n", sl->name, (char *)sl->charp); \
			else \
			    pfmt(stdout, MM_NOSTD, ":216:%-15s \"(standard output)\"\n", sl->name); \
		    }

    if (argc == 1) {
	for (tl = Togglelist; tl->name; tl++) {
	    dotog(tl);
	}
	pfmt(stdout, MM_NOSTD, ":39:\n");
	for (sl = Setlist; sl->name; sl++) {
	    doset(sl);
	}
    } else {
	int i;

	for (i = 1; i < argc; i++) {
	    sl = getset(argv[i]);
	    tl = GETTOGGLE(argv[i]);
	    if (Ambiguous(sl) || Ambiguous(tl)) {
		pfmt(stdout, MM_NOSTD, ":219:?Ambiguous argument '%s'.\n", argv[i]);
		return 0;
	    } else if (!sl && !tl) {
		pfmt(stdout, MM_NOSTD, ":220:?Unknown argument '%s'.\n", argv[i]);
		return 0;
	    } else {
		if (tl) {
		    dotog(tl);
		}
		if (sl) {
		    doset(sl);
		}
	    }
	}
    }
/*@*/optionstatus();
#if	defined(ENCRYPT)
    EncryptStatus();
#endif
    return 1;
#undef	doset
#undef	dotog
}

/*
 * The following are the data structures, and many of the routines,
 * relating to command processing.
 */

/*
 * Set the escape character.
 */
	static int
setescape(argc, argv)
	int argc;
	char *argv[];
{
	register char *arg;
	char buf[50];

	pfmt(stdout, MM_NOSTD,
	    ":221:Deprecated usage - please use 'set escape%s%s' in the future.\n",
				(argc > 2)? " ":"", (argc > 2)? argv[1]: "");
	if (argc > 2)
		arg = argv[1];
	else {
		pfmt(stdout, MM_NOSTD, ":222:new escape character: ");
		(void) fgets(buf, sizeof(buf), stdin);
		arg = buf;
	}
	if (arg[0] != '\0')
		escape = arg[0];
	if (!In3270) {
		pfmt(stdout, MM_NOSTD, ":223:Escape character is '%s'.\n", control(escape));
	}
	(void) fflush(stdout);
	return 1;
}

    /*VARARGS*/
    static int
togcrmod()
{
    crmod = !crmod;
    pfmt(stdout, MM_NOSTD, ":224:Deprecated usage - please use 'toggle crmod' in the future.\n");
    if (crmod)
	pfmt(stdout, MM_NOSTD, ":225:Will map carriage return on output.\n");
    else
	pfmt(stdout, MM_NOSTD, ":226:Won't map carriage return on output.\n");
    (void) fflush(stdout);
    return 1;
}

    /*VARARGS*/
    int
suspend()
{
#ifdef	SIGTSTP
    setcommandmode();
    {
	long oldrows, oldcols, newrows, newcols, err;

	err = TerminalWindowSize(&oldrows, &oldcols);
	(void) kill(0, SIGTSTP);
	err += TerminalWindowSize(&newrows, &newcols);
	if (connected && !err &&
	    ((oldrows != newrows) || (oldcols != newcols))) {
		sendnaws();
	}
    }
    /* reget parameters in case they were changed */
    TerminalSaveState();
    setconnmode(0);
#else
    pfmt(stdout, MM_NOSTD, ":227:Suspend is not supported.  Try the '!' command instead\n");
#endif
    return 1;
}

#if	!defined(TN3270)
    /*ARGSUSED*/
    int
shell(argc, argv)
    int argc;
    char *argv[];
{
    setcommandmode();
#ifdef SYSV
#define vfork fork
#endif
    switch(vfork()) {
    case -1:
	pfmt(stderr, MM_ERROR, ":228:Fork failed: %s\n", strerror(errno));
	break;

    case 0:
	{
	    /*
	     * Fire up the shell in the child.
	     */
	    register char *shellp, *shellname;

	    shellp = getenv("SHELL");
	    if (shellp == NULL)
		shellp = "/bin/sh";
	    if ((shellname = rindex(shellp, '/')) == 0)
		shellname = shellp;
	    else
		shellname++;
	    if (argc > 1)
		execl(shellp, shellname, "-c", &saveline[1], 0);
	    else
		execl(shellp, shellname, 0);
	    pfmt(stderr, MM_ERROR, ":229:Execl: %s\n", strerror(errno));
	    _exit(1);
	}
    default:
	    (void)wait((int *)0);	/* Wait for the shell to complete */
    }
    return 1;
}
#endif	/* !defined(TN3270) */

    /*VARARGS*/
    static
bye(argc, argv)
    int  argc;		/* Number of arguments */
    char *argv[];	/* arguments */
{
    extern int resettermname;

    if (connected) {
	(void) shutdown(net, 2);
	pfmt(stdout, MM_NOSTD, ":230:Connection closed.\n");
	(void) NetClose(net);
	connected = 0;
	resettermname = 1;
#if	defined(AUTHENTICATE) || defined(ENCRYPT)
	auth_encrypt_connect(connected);
#endif
	/* reset options */
	tninit();
#if	defined(TN3270)
	SetIn3270();		/* Get out of 3270 mode */
#endif	/* defined(TN3270) */
    }
    if ((argc != 2) || (strcmp(argv[1], "fromquit") != 0)) {
	longjmp(toplevel, 1);
	/* NOTREACHED */
    }
    return 1;			/* Keep lint, etc., happy */
}

/*VARARGS*/
quit()
{
	(void) call(bye, "bye", "fromquit", 0);
	Exit(0);
	/*NOTREACHED*/
}

/*VARARGS*/
	int
logout()
{
	send_do(TELOPT_LOGOUT, 1);
	(void) netflush();
	return 1;
}


/*
 * The SLC command.
 */

struct slclist {
	char	*name;
	char	*help;
	void	(*handler)();
	int	arg;
};

extern void slc_help();

struct slclist SlcList[] = {
    { "export",	":231:Use local special character definitions",
						slc_mode_export,	0 },
    { "import",	":232:Use remote special character definitions",
						slc_mode_import,	1 },
    { "check",	":233:Verify remote special character definitions",
						slc_mode_import,	0 },
    { "help",	0,				slc_help,		0 },
    { "?",	":211:Print help information",	slc_help,		0 },
    { 0 },
};

    static void
slc_help()
{
    struct slclist *c;

    for (c = SlcList; c->name; c++) {
	if (c->help) {
	    if (*c->help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (c->help && *c->help)
		    pfmt(stdout, MM_NOSTD, c->help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
}

    static struct slclist *
getslc(name)
    char *name;
{
    return (struct slclist *)
		genget(name, (char **) SlcList, sizeof(struct slclist));
}

    static
slccmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct slclist *c;

    if (argc != 2) {
	pfmt(stderr, MM_ERROR,
	    ":234:Need an argument to 'slc' command.  'slc ?' for help.\n");
	return 0;
    }
    c = getslc(argv[1]);
    if (c == 0) {
        pfmt(stderr, MM_NOSTD, ":235:'%s': unknown argument ('slc ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (Ambiguous(c)) {
        pfmt(stderr, MM_NOSTD, ":236:'%s': ambiguous argument ('slc ?' for help).\n",
    				argv[1]);
        return 0;
    }
    (*c->handler)(c->arg);
    slcstate();
    return 1;
}

/*
 * The ENVIRON command.
 */

struct envlist {
	char	*name;
	char	*help;
	void	(*handler)();
	int	narg;
};

extern struct env_lst *
	env_define P((unsigned char *, unsigned char *));
extern void
	env_undefine P((unsigned char *)),
	env_export P((unsigned char *)),
	env_unexport P((unsigned char *)),
	env_send P((unsigned char *)),
	env_list P((void)),
	env_help P((void));

struct envlist EnvList[] = {
    { "define",	":237:Define an environment variable",
						(void (*)())env_define,	2 },
    { "undefine", ":238:Undefine an environment variable",
						env_undefine,	1 },
    { "export",	":239:Mark an environment variable for automatic export",
						env_export,	1 },
    { "unexport", ":240:Don't mark an environment variable for automatic export",
						env_unexport,	1 },
    { "send",	":241:Send an environment variable", env_send,	1 },
    { "list",	":242:List the current environment variables",
						env_list,	0 },
    { "help",	0,				env_help,		0 },
    { "?",	":211:Print help information",	env_help,		0 },
    { 0 },
};

    static void
env_help()
{
    struct envlist *c;

    for (c = EnvList; c->name; c++) {
	if (c->help) {
	    if (*c->help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (c->help && *c->help)
		    pfmt(stdout, MM_NOSTD, c->help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
}

    static struct envlist *
getenvcmd(name)
    char *name;
{
    return (struct envlist *)
		genget(name, (char **) EnvList, sizeof(struct envlist));
}

env_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct envlist *c;

    if (argc < 2) {
	pfmt(stderr, MM_ERROR,
	    ":243:Need an argument to 'environ' command.  'environ ?' for help.\n");
	return 0;
    }
    c = getenvcmd(argv[1]);
    if (c == 0) {
        pfmt(stderr, MM_ERROR, ":244:'%s': unknown argument ('environ ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (Ambiguous(c)) {
        pfmt(stderr, MM_ERROR, ":245:'%s': ambiguous argument ('environ ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (c->narg + 2 != argc) {
	if (c->narg < argc + 2) {
	    if (c->narg == 1)
		pfmt(stderr, MM_ERROR, ":246:Need only 1 argument to 'environ %s' command.  'environ ?' for help.\n", c->name);
	    else
		pfmt(stderr, MM_ERROR, ":247:Need only %d arguments to 'environ %s' command.  'environ ?' for help.\n", c->narg, c->name);
	}
	else {
	    if (c->narg == 1)
		pfmt(stderr, MM_ERROR, ":248:Need 1 argument to 'environ %s' command.  'environ ?' for help.\n", c->name);
	    else
		pfmt(stderr, MM_ERROR, ":249:Need %d arguments to 'environ %s' command.  'environ ?' for help.\n", c->narg, c->name);
	}
	return 0;
    }
    (*c->handler)(argv[2], argv[3]);
    return 1;
}

struct env_lst {
	struct env_lst *next;	/* pointer to next structure */
	struct env_lst *prev;	/* pointer to next structure */
	unsigned char *var;	/* pointer to variable name */
	unsigned char *value;	/* pointer to varialbe value */
	int export;		/* 1 -> export with default list of variables */
};

struct env_lst envlisthead;

	struct env_lst *
env_find(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	for (ep = envlisthead.next; ep; ep = ep->next) {
		if (strcmp((char *)ep->var, (char *)var) == 0)
			return(ep);
	}
	return(NULL);
}

	void
env_init()
{
	extern char **environ;
	register char **epp, *cp;
	register struct env_lst *ep;

	for (epp = environ; *epp; epp++) {
		if (cp = index(*epp, '=')) {
			*cp = '\0';
			ep = env_define((unsigned char *)*epp,
					(unsigned char *)cp+1);
			ep->export = 0;
			*cp = '=';
		}
	}
	/*
	 * Special case for DISPLAY variable.  If it is ":0.0" or
	 * "unix:0.0", we have to get rid of "unix" and insert our
	 * hostname.
	 */
	if ((ep = env_find("DISPLAY"))
	    && ((*ep->value == ':')
	        || (strncmp((char *)ep->value, "unix:", 5) == 0))) {
		char hbuf[256+1];
		char *cp2 = index((char *)ep->value, ':');

		gethostname(hbuf, 256);
		hbuf[256] = '\0';
		cp = (char *)malloc(strlen(hbuf) + strlen(cp2) + 1);
		sprintf((char *)cp, "%s%s", hbuf, cp2);
		free(ep->value);
		ep->value = (unsigned char *)cp;
	}
	/*
	 * If USER is not defined, but LOGNAME is, then add
	 * USER with the value from LOGNAME.  By default, we
	 * don't export the USER variable.
	 */
	if ((env_find("USER") == NULL) && (ep = env_find("LOGNAME"))) {
		env_define((unsigned char *)"USER", ep->value);
		env_unexport((unsigned char *)"USER");
	}
	env_export((unsigned char *)"DISPLAY");
	env_export((unsigned char *)"PRINTER");
}

	struct env_lst *
env_define(var, value)
	unsigned char *var, *value;
{
	register struct env_lst *ep;

	if (ep = env_find(var)) {
		if (ep->var)
			free(ep->var);
		if (ep->value)
			free(ep->value);
	} else {
		ep = (struct env_lst *)malloc(sizeof(struct env_lst));
		ep->next = envlisthead.next;
		envlisthead.next = ep;
		ep->prev = &envlisthead;
		if (ep->next)
			ep->next->prev = ep;
	}
	ep->export = 1;
	ep->var = (unsigned char *)strdup((char *)var);
	ep->value = (unsigned char *)strdup((char *)value);
	return(ep);
}

	void
env_undefine(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var)) {
		ep->prev->next = ep->next;
		if (ep->next)
			ep->next->prev = ep->prev;
		if (ep->var)
			free(ep->var);
		if (ep->value)
			free(ep->value);
		free(ep);
	}
}

	void
env_export(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		ep->export = 1;
}

	void
env_unexport(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		ep->export = 0;
}

	void
env_send(var)
	unsigned char *var;
{
	register struct env_lst *ep;

        if (my_state_is_wont(TELOPT_ENVIRON)) {
		pfmt(stderr, MM_ERROR,
		    ":250:Cannot send '%s': Telnet ENVIRON option not enabled\n",
									var);
		return;
	}
	ep = env_find(var);
	if (ep == 0) {
		pfmt(stderr, MM_ERROR, ":251:Cannot send '%s': variable not defined\n",
									var);
		return;
	}
	env_opt_start_info();
	env_opt_add(ep->var);
	env_opt_end(0);
}

	void
env_list()
{
	register struct env_lst *ep;

	for (ep = envlisthead.next; ep; ep = ep->next) {
		pfmt(stdout, MM_NOSTD, ":252:%c %-20s %s\n", ep->export ? '*' : ' ',
					ep->var, ep->value);
	}
}

	unsigned char *
env_default(init)
	int init;
{
	static struct env_lst *nep = NULL;

	if (init) {
		nep = &envlisthead;
		return;
	}
	if (nep) {
		while (nep = nep->next) {
			if (nep->export)
				return(nep->var);
		}
	}
	return(NULL);
}

	unsigned char *
env_getvalue(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		return(ep->value);
	return(NULL);
}

#if	defined(AUTHENTICATE)
/*
 * The AUTHENTICATE command.
 */

struct authlist {
	char	*name;
	char	*help;
	int	(*handler)();
	int	narg;
};

extern int
	auth_enable P((int)),
	auth_disable P((int)),
	auth_status P((void)),
	auth_help P((void));

struct authlist AuthList[] = {
    { "status",	":253:Display current status of authentication information",
						auth_status,	0 },
    { "disable", ":254:Disable an authentication type ('auth disable ?' for more)",
						auth_disable,	1 },
    { "enable", ":255:Enable an authentication type ('auth enable ?' for more)",
						auth_enable,	1 },
    { "help",	0,				auth_help,		0 },
    { "?",	":211:Print help information",	auth_help,		0 },
    { 0 },
};

    static int
auth_help()
{
    struct authlist *c;

    for (c = AuthList; c->name; c++) {
	if (c->help) {
	    if (*c->help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (c->help && *c->help)
		    pfmt(stdout, MM_NOSTD, c->help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
    return 0;
}

auth_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct authlist *c;

    c = (struct authlist *)
		genget(argv[1], (char **) AuthList, sizeof(struct authlist));
    if (c == 0) {
        pfmt(stderr, MM_ERROR, ":256:'%s': unknown argument ('auth ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (Ambiguous(c)) {
        pfmt(stderr, MM_ERROR, ":257:'%s': ambiguous argument ('auth ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (c->narg + 2 != argc) {
	if (c->narg < argc + 2) {
	    if (c->narg == 1)
		pfmt(stderr, MM_ERROR, ":258:Need only 1 argument to 'auth %s' command.  'auth ?' for help.\n", c->name);
	    else
		pfmt(stderr, MM_ERROR, ":259:Need only %d arguments to 'auth %s' command.  'auth ?' for help.\n", c->narg, c->name);
	}
	else {
	    if (c->narg == 1)
		pfmt(stderr, MM_ERROR, ":260:Need 1 argument to 'auth %s' command.  'auth ?' for help.\n", c->name);
	    else
		pfmt(stderr, MM_ERROR, ":261:Need %d arguments to 'auth %s' command.  'auth ?' for help.\n", c->narg, c->name);
	}
	return 0;
    }
    return((*c->handler)(argv[2], argv[3]));
}
#endif

#if	defined(ENCRYPT)
/*
 * The ENCRYPT command.
 */

struct encryptlist {
	char	*name;
	char	*help;
	int	(*handler)();
	int	needconnect;
	int	minarg;
	int	maxarg;
};

extern int
	EncryptEnable P((char *, char *)),
	EncryptDisable P((char *, char *)),
	EncryptType P((char *, char *)),
	EncryptStart P((char *)),
	EncryptStartInput P((void)),
	EncryptStartOutput P((void)),
	EncryptStop P((char *)),
	EncryptStopInput P((void)),
	EncryptStopOutput P((void)),
	EncryptStatus P((void)),
	EncryptHelp P((void));

struct encryptlist EncryptList[] = {
    { "enable", ":262:Enable encryption. ('encrypt enable ?' for more)",
						EncryptEnable, 1, 1, 2 },
    { "disable", ":263:Disable encryption. ('encrypt enable ?' for more)",
						EncryptDisable, 0, 1, 2 },
    { "type", ":264:Set encryptiong type. ('encrypt type ?' for more)",
						EncryptType, 0, 1, 1 },
    { "start", ":265:Start encryption. ('encrypt start ?' for more)",
						EncryptStart, 1, 0, 1 },
    { "stop", ":266:Stop encryption. ('encrypt stop ?' for more)",
						EncryptStop, 1, 0, 1 },
    { "input", ":267:Start encrypting the input stream",
						EncryptStartInput, 1, 0, 0 },
    { "-input", ":268:Stop encrypting the input stream",
						EncryptStopInput, 1, 0, 0 },
    { "output", ":269:Start encrypting the output stream",
						EncryptStartOutput, 1, 0, 0 },
    { "-output", ":270:Stop encrypting the output stream",
						EncryptStopOutput, 1, 0, 0 },

    { "status",	":253:Display current status of authentication information",
						EncryptStatus,	0, 0, 0 },
    { "help",	0,				EncryptHelp,	0, 0, 0 },
    { "?",	":211:Print help information",	EncryptHelp,	0, 0, 0 },
    { 0 },
};

    static int
EncryptHelp()
{
    struct encryptlist *c;

    for (c = EncryptList; c->name; c++) {
	if (c->help) {
	    if (*c->help) {
		pfmt(stdout, MM_NOSTD, ":42:%-15s ", c->name);
		if (c->help && *c->help)
		    pfmt(stdout, MM_NOSTD, c->help);
		pfmt(stdout, MM_NOSTD, ":39:\n");
	    }
	    else
		pfmt(stdout, MM_NOSTD, ":39:\n");
	}
    }
    return 0;
}

encrypt_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct encryptlist *c;

    c = (struct encryptlist *)
		genget(argv[1], (char **) EncryptList, sizeof(struct encryptlist));
    if (c == 0) {
        pfmt(stderr, MM_ERROR, ":271:'%s': unknown argument ('encrypt ?' for help).\n",
    				argv[1]);
        return 0;
    }
    if (Ambiguous(c)) {
        pfmt(stderr, MM_ERROR, ":272:'%s': ambiguous argument ('encrypt ?' for help).\n",
    				argv[1]);
        return 0;
    }
    argc -= 2;
    if (argc < c->minarg || argc > c->maxarg) {
	if (c->minarg == c->maxarg) {
	    if (c->minarg < argc) {
		if (c->minarg == 1)
		    pfmt(stderr, MM_ERROR, ":273:Need only 1 argument to 'encrypt %s' command.  'encrypt ?' for help.\n", c->name);
		else
		    pfmt(stderr, MM_ERROR, ":274:Need only %d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n", c->minarg, c->name);
	    }
	    else {
		if (c->minarg == 1)
		    pfmt(stderr, MM_ERROR, ":275:Need 1 argument to 'encrypt %s' command.  'encrypt ?' for help.\n", c->name);
		else
		    pfmt(stderr, MM_ERROR, ":276:Need %d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n", c->minarg, c->name);
	    }
	} else {
	    if (c->maxarg < argc)
		pfmt(stderr, MM_ERROR, ":277:Need only %d-%d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n", c->minarg, c->maxarg, c->name);
	    else
		pfmt(stderr, MM_ERROR, ":278:Need %d-%d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n", c->minarg, c->maxarg, c->name);
	}
	return 0;
    }
    if (c->needconnect && !connected) {
	if (!(argc && (isprefix(argv[2], "help") || isprefix(argv[2], "?")))) {
	    pfmt(stdout, MM_NOSTD, ":29:?Need to be connected first.\n");
	    return 0;
	}
    }
    return ((*c->handler)(argc > 0 ? argv[2] : 0,
			argc > 1 ? argv[3] : 0,
			argc > 2 ? argv[4] : 0));
}
#endif

#if	defined(unix) && defined(TN3270)
    static void
filestuff(fd)
    int fd;
{
    int res;

#ifdef	F_GETOWN
    setconnmode(0);
    res = fcntl(fd, F_GETOWN, 0);
    setcommandmode();

    if (res == -1) {
	pfmt(stderr, MM_ERROR, ":279:fcntl: %s\n", strerror(errno));
	return;
    }
    pfmt(stdout, MM_NOSTD, ":280:\tOwner is %d.\n", res);
#endif

    setconnmode(0);
    res = fcntl(fd, F_GETFL, 0);
    setcommandmode();

    if (res == -1) {
	pfmt(stderr, MM_ERROR, ":279:fcntl: %s\n", strerror(errno));
	return;
    }
    pfmt(stdout, MM_NOSTD, ":281:\tFlags are 0x%x: %s\n", res, decodeflags(res));
}
#endif /* defined(unix) && defined(TN3270) */

/*
 * Print status about the connection.
 */
    /*ARGSUSED*/
    static
status(argc, argv)
    int	 argc;
    char *argv[];
{
    if (connected) {
	pfmt(stdout, MM_NOSTD, ":282:Connected to %s.\n", hostname);
	if ((argc < 2) || strcmp(argv[1], "notmuch")) {
	    int mode = getconnmode();

	    if (my_want_state_is_will(TELOPT_LINEMODE)) {
		pfmt(stdout, MM_NOSTD, ":283:Operating with LINEMODE option\n");
		if (mode&MODE_EDIT)
		    pfmt(stdout, MM_NOSTD, ":284:Local line editing\n");
		else
		    pfmt(stdout, MM_NOSTD, ":285:No line editing\n");
		if (mode&MODE_TRAPSIG)
		    pfmt(stdout, MM_NOSTD, ":286:Local catching of signals\n");
		else
		    pfmt(stdout, MM_NOSTD, ":287:No catching of signals\n");
		slcstate();
#ifdef	KLUDGELINEMODE
	    } else if (kludgelinemode && my_want_state_is_dont(TELOPT_SGA)) {
		pfmt(stdout, MM_NOSTD, ":288:Operating in obsolete linemode\n");
#endif
	    } else {
		pfmt(stdout, MM_NOSTD, ":289:Operating in single character mode\n");
		if (localchars)
		    pfmt(stdout, MM_NOSTD, ":290:Catching signals locally\n");
	    }
	    if (mode&MODE_ECHO)
		pfmt(stdout, MM_NOSTD, ":291:Local character echo\n");
	    else
		pfmt(stdout, MM_NOSTD, ":292:Remote character echo\n");
	    if (my_want_state_is_will(TELOPT_LFLOW)) {
		if (mode&MODE_FLOW)
		    pfmt(stdout, MM_NOSTD, ":293:Local flow control\n");
		else
		    pfmt(stdout, MM_NOSTD, ":294:No flow control\n");
	    }
#if	defined(ENCRYPT)
	    encrypt_display();
#endif
	}
    } else {
	pfmt(stdout, MM_NOSTD, ":295:No connection.\n");
    }
#   if !defined(TN3270)
    if (rlogin == _POSIX_VDISABLE )
	    pfmt(stdout, MM_NOSTD, ":223:Escape character is '%s'.\n", control(escape));
    (void) fflush(stdout);
#   else /* !defined(TN3270) */
    if ((!In3270) && ((argc < 2) || strcmp(argv[1], "notmuch"))) {
	pfmt(stdout, MM_NOSTD, ":223:Escape character is '%s'.\n", control(escape));
    }
#   if defined(unix)
    if ((argc >= 2) && !strcmp(argv[1], "everything")) {
	if (sigiocount == 1)
	    pfmt(stdout, MM_NOSTD, ":296:SIGIO received 1 time.\n");
	else
	    pfmt(stdout, MM_NOSTD, ":297:SIGIO received %d times.\n", sigiocount);
	if (In3270) {
	    pfmt(stdout, MM_NOSTD, ":298:Process ID %d, process group %d.\n",
					    getpid(), getpgrp(getpid()));
	    pfmt(stdout, MM_NOSTD, ":299:Terminal input:\n");
	    filestuff(tin);
	    pfmt(stdout, MM_NOSTD, ":300:Terminal output:\n");
	    filestuff(tout);
	    pfmt(stdout, MM_NOSTD, ":301:Network socket:\n");
	    filestuff(net);
	}
    }
    if (In3270 && transcom) {
       pfmt(stdout, MM_NOSTD, ":302:Transparent mode command is '%s'.\n", transcom);
    }
#   endif /* defined(unix) */
    (void) fflush(stdout);
    if (In3270) {
	return 0;
    }
#   endif /* defined(TN3270) */
    return 1;
}

#ifdef	SIGINFO
/*
 * Function that gets called when SIGINFO is received.
 */
ayt_status()
{
    (void) call(status, "status", "notmuch", 0);
}
#endif

    int
tn(argc, argv)
    int argc;
    char *argv[];
{
    register struct hostent *host = 0;
    struct sockaddr_in sin;
    struct servent *sp = 0;
    unsigned long temp, inet_addr();
    extern char *inet_ntoa();
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    char *srp = 0, *strrchr();
    unsigned long sourceroute(), srlen;
#endif
    char *cmd, *hostp = 0, *portp = 0, *user = 0;

    /* clear the socket address prior to use */
    bzero((char *)&sin, sizeof(sin));

    if (connected) {
	pfmt(stdout, MM_NOSTD, ":303:?Already connected to %s\n", hostname);
	setuid(getuid());
	return 0;
    }
    if (argc < 2) {
	(void) strcpy(line, "open ");
	pfmt(stdout, MM_NOSTD, ":304:(to) ");
	(void) fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
	makeargv();
	argc = margc;
	argv = margv;
    }
    cmd = *argv;
    --argc; ++argv;
    while (argc) {
	if (isprefix(*argv, "help") || isprefix(*argv, "?"))
	    goto usage;
	if (strcmp(*argv, "-l") == 0) {
	    --argc; ++argv;
	    if (argc == 0)
		goto usage;
	    user = *argv++;
	    --argc;
	    continue;
	}
	if (strcmp(*argv, "-a") == 0) {
	    --argc; ++argv;
	    autologin = 1;
	    continue;
	}
	if (hostp == 0) {
	    hostp = *argv++;
	    --argc;
	    continue;
	}
	if (portp == 0) {
	    portp = *argv++;
	    --argc;
	    continue;
	}
    usage:
	pfmt(stdout, MM_NOSTD, ":305:usage: %s [-l user] [-a] host-name [port]\n", cmd);
	setuid(getuid());
	return 0;
    }
    if (hostp == 0)
	goto usage;

#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    if (hostp[0] == '@' || hostp[0] == '!') {
	if ((hostname = strrchr(hostp, ':')) == NULL)
	    hostname = strrchr(hostp, '@');
	hostname++;
	srp = 0;
	temp = sourceroute(hostp, &srp, &srlen);
	if (temp == 0) {
	    herror(srp);
	    setuid(getuid());
	    return 0;
	} else if (temp == -1) {
	    pfmt(stdout, MM_NOSTD, ":306:Bad source route option: %s\n", hostp);
	    setuid(getuid());
	    return 0;
	} else {
	    sin.sin_addr.s_addr = temp;
	    sin.sin_family = AF_INET;
	}
    } else {
#endif
	temp = inet_addr(hostp);
	if (temp != (unsigned long) -1) {
	    sin.sin_addr.s_addr = temp;
	    sin.sin_family = AF_INET;
	    (void) strcpy(_hostname, hostp);
	    hostname = _hostname;
	} else {
	    host = gethostbyname(hostp);
	    if (host) {
		sin.sin_family = host->h_addrtype;
#if	defined(h_addr)		/* In 4.3, this is a #define */
		memcpy((caddr_t)&sin.sin_addr,
				host->h_addr_list[0], host->h_length);
#else	/* defined(h_addr) */
		memcpy((caddr_t)&sin.sin_addr, host->h_addr, host->h_length);
#endif	/* defined(h_addr) */
		strncpy(_hostname, host->h_name, sizeof(_hostname));
		_hostname[sizeof(_hostname)-1] = '\0';
		hostname = _hostname;
	    } else {
		herror(hostp);
	        setuid(getuid());
		return 0;
	    }
	}
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    }
#endif
    if (portp) {
	if (*portp == '-') {
	    portp++;
	    telnetport = 1;
	} else
	    telnetport = 0;
	sin.sin_port = atoi(portp);
	if (sin.sin_port == 0) {
	    sp = getservbyname(portp, "tcp");
	    if (sp)
		sin.sin_port = sp->s_port;
	    else {
		pfmt(stdout, MM_NOSTD, ":307:%s: bad port number\n", portp);
	        setuid(getuid());
		return 0;
	    }
	} else {
#if	!defined(htons)
	    u_short htons();
#endif	/* !defined(htons) */
	    sin.sin_port = htons(sin.sin_port);
	}
    } else {
	if (sp == 0) {
	    sp = getservbyname("telnet", "tcp");
	    if (sp == 0) {
		pfmt(stderr, MM_ERROR, ":308:tcp/telnet: unknown service\n");
	        setuid(getuid());
		return 0;
	    }
	    sin.sin_port = sp->s_port;
	}
	telnetport = 1;
    }
    pfmt(stdout, MM_NOSTD, ":309:Trying %s...\n", inet_ntoa(sin.sin_addr));
    do {
	net = socket(AF_INET, SOCK_STREAM, 0);
	setuid(getuid());
	if (net < 0) {
	    pfmt(stderr, MM_ERROR, ":310:socket: %s\n", strerror(errno));
	    return 0;
	}
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
	if (srp && setsockopt(net, IPPROTO_IP, IP_OPTIONS, (char *)srp, srlen) < 0)
		pfmt(stderr, MM_ERROR, ":311:setsockopt (IP_OPTIONS): %s\n", strerror(errno));
#endif
#if	defined(IPPROTO_IP) && defined(IP_TOS)
	{
# if	defined(HAS_GETTOS)
	    struct tosent *tp;
	    if (tos < 0 && (tp = gettosbyname("telnet", "tcp")))
		tos = tp->t_tos;
# endif
	    if (tos < 0)
		tos = 020;	/* Low Delay bit */
	    if (tos
		&& (setsockopt(net, IPPROTO_IP, IP_TOS, &tos, sizeof(int)) < 0)
		&& (errno != ENOPROTOOPT))
		    pfmt(stderr, MM_ERROR, ":312:setsockopt (IP_TOS) (ignored): %s\n", strerror(errno));
	}
#endif	/* defined(IPPROTO_IP) && defined(IP_TOS) */

	if (debug && SetSockOpt(net, SOL_SOCKET, SO_DEBUG, 1) < 0) {
		pfmt(stderr, MM_ERROR, ":43:setsockopt (SO_DEBUG): %s\n", strerror(errno));
	}

	if (connect(net, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
#if	defined(h_addr)		/* In 4.3, this is a #define */
	    if (host && host->h_addr_list[1]) {
		int oerrno = errno;

		pfmt(stderr, MM_ERROR, ":313:connect to address %s: %s\n",
						inet_ntoa(sin.sin_addr), strerror(errno));
		errno = oerrno;
		fprintf(stderr,strerror(errno));
		host->h_addr_list++;
		memcpy((caddr_t)&sin.sin_addr, 
			host->h_addr_list[0], host->h_length);
		(void) NetClose(net);
		continue;
	    }
#endif	/* defined(h_addr) */
	    pfmt(stderr, MM_ERROR, ":314:Unable to connect to remote host: %s\n", strerror(errno));
	    return 0;
	}
	connected++;
#if	defined(AUTHENTICATE) || defined(ENCRYPT)
	auth_encrypt_connect(connected);
#endif
    } while (connected == 0);
    cmdrc(hostp, hostname);
    if (autologin && user == NULL) {
	struct passwd *pw;

	user = getenv("USER");
	if (user == NULL ||
	    (pw = getpwnam(user)) && pw->pw_uid != getuid()) {
		if (pw = getpwuid(getuid()))
			user = pw->pw_name;
		else
			user = NULL;
	}
    }
    if (user) {
	env_define((unsigned char *)"USER", (unsigned char *)user);
	env_export((unsigned char *)"USER");
    }
    (void) call(status, "status", "notmuch", 0);
    if (setjmp(peerdied) == 0)
	telnet(user);
    (void) NetClose(net);
    ExitString(":315:Connection closed by foreign host.\n",1);
    /*NOTREACHED*/
}

#define HELPINDENT (sizeof ("connect"))

static char
	openhelp[] =	":316:connect to a site",
	closehelp[] =	":317:close current connection",
	logouthelp[] =	":318:forcibly logout remote user and close the connection",
	quithelp[] =	":319:exit telnet",
	statushelp[] =	":320:print status information",
	helphelp[] =	":321:print help information",
	sendhelp[] =	":322:transmit special characters ('send ?' for more)",
	sethelp[] = 	":323:set operating parameters ('set ?' for more)",
	unsethelp[] = 	":324:unset operating parameters ('unset ?' for more)",
	togglestring[] =":325:toggle operating parameters ('toggle ?' for more)",
	slchelp[] =	":326:change state of special charaters ('slc ?' for more)",
	displayhelp[] =	":327:display operating parameters",
#if	defined(TN3270) && defined(unix)
	transcomhelp[] = ":328:specify Unix command for transparent mode pipe",
#endif	/* defined(TN3270) && defined(unix) */
#if	defined(AUTHENTICATE)
	authhelp[] =	":329:turn on (off) authentication ('auth ?' for more)",
#endif
#if	defined(ENCRYPT)
	encrypthelp[] =	":330:turn on (off) encryption ('encrypt ?' for more)",
#endif
#if	defined(unix)
	zhelp[] =	":331:suspend telnet",
#endif	/* defined(unix) */
	shellhelp[] =	":332:invoke a subshell",
	envhelp[] =	":333:change environment variables ('environ ?' for more)",
	modestring[] = ":334:try to enter line or character mode ('mode ?' for more)";

extern int	help();

static Command cmdtab[] = {
	{ "close",	closehelp,	bye,		1 },
	{ "logout",	logouthelp,	logout,		1 },
	{ "display",	displayhelp,	display,	0 },
	{ "mode",	modestring,	modecmd,	0 },
	{ "open",	openhelp,	tn,		0 },
	{ "quit",	quithelp,	quit,		0 },
	{ "send",	sendhelp,	sendcmd,	0 },
	{ "set",	sethelp,	setcmd,		0 },
	{ "unset",	unsethelp,	unsetcmd,	0 },
	{ "status",	statushelp,	status,		0 },
	{ "toggle",	togglestring,	toggle,		0 },
	{ "slc",	slchelp,	slccmd,		0 },
#if	defined(TN3270) && defined(unix)
	{ "transcom",	transcomhelp,	settranscom,	0 },
#endif	/* defined(TN3270) && defined(unix) */
#if	defined(AUTHENTICATE)
	{ "auth",	authhelp,	auth_cmd,	0 },
#endif
#if	defined(ENCRYPT)
	{ "encrypt",	encrypthelp,	encrypt_cmd,	0 },
#endif
#if	defined(unix)
	{ "z",		zhelp,		suspend,	0 },
#endif	/* defined(unix) */
#if	defined(TN3270)
	{ "!",		shellhelp,	shell,		1 },
#else
	{ "!",		shellhelp,	shell,		0 },
#endif
	{ "environ",	envhelp,	env_cmd,	0 },
	{ "?",		helphelp,	help,		0 },
	0
};

static char	crmodhelp[] =	":335:deprecated command -- use 'toggle crmod' instead";
static char	escapehelp[] =	":336:deprecated command -- use 'set escape' instead";

static Command cmdtab2[] = {
	{ "help",	0,		help,		0 },
	{ "escape",	escapehelp,	setescape,	0 },
	{ "crmod",	crmodhelp,	togcrmod,	0 },
	0
};


/*
 * Call routine with argc, argv set from args (terminated by 0).
 */

    /*VARARGS1*/
    static
call(va_alist)
    va_dcl
{
    va_list ap;
    typedef int (*intrtn_t)();
    intrtn_t routine;
    char *args[100];
    int argno = 0;

    va_start(ap);
    routine = (va_arg(ap, intrtn_t));
    while ((args[argno++] = va_arg(ap, char *)) != 0) {
	;
    }
    va_end(ap);
    return (*routine)(argno-1, args);
}


    static Command *
getcmd(name)
    char *name;
{
    Command *cm;

    if (cm = (Command *) genget(name, (char **) cmdtab, sizeof(Command)))
	return cm;
    return (Command *) genget(name, (char **) cmdtab2, sizeof(Command));
}

    void
command(top, tbuf, cnt)
    int top;
    char *tbuf;
    int cnt;
{
    register Command *c;

    setcommandmode();
    if (!top) {
	putchar('\n');
#if	defined(unix)
    } else {
	(void) sigset(SIGINT, SIG_DFL);
	(void) sigset(SIGQUIT, SIG_DFL);
#endif	/* defined(unix) */
    }
    for (;;) {
	if (rlogin == _POSIX_VDISABLE)
		pfmt(stdout, MM_NOSTD, ":337:%s> ", prompt);
	if (tbuf) {
	    register char *cp;
	    cp = line;
	    while (cnt > 0 && (*cp++ = *tbuf++) != '\n')
		cnt--;
	    tbuf = 0;
	    if (cp == line || *--cp != '\n' || cp == line)
		goto getline;
	    *cp = '\0';
	    if (rlogin == _POSIX_VDISABLE)
	        pfmt(stdout, MM_NOSTD, ":338:%s\n", line);
	} else {
	getline:
	    if (rlogin != _POSIX_VDISABLE)
		pfmt(stdout, MM_NOSTD, ":337:%s> ", prompt);
	    if (fgets(line, sizeof(line), stdin) == NULL) {
		if (feof(stdin) || ferror(stdin)) {
		    (void) quit();
		    /*NOTREACHED*/
		}
		break;
	    }
	}
	if (line[0] == 0)
	    break;
	makeargv();
	if (margv[0] == 0) {
	    break;
	}
	c = getcmd(margv[0]);
	if (Ambiguous(c)) {
	    pfmt(stdout, MM_NOSTD, ":339:?Ambiguous command\n");
	    continue;
	}
	if (c == 0) {
	    pfmt(stdout, MM_NOSTD, ":340:?Invalid command\n");
	    continue;
	}
	if (c->needconnect && !connected) {
	    pfmt(stdout, MM_NOSTD, ":29:?Need to be connected first.\n");
	    continue;
	}
	if ((*c->handler)(margc, margv)) {
	    break;
	}
    }
    if (!top) {
	if (!connected) {
	    longjmp(toplevel, 1);
	    /*NOTREACHED*/
	}
#if	defined(TN3270)
	if (shell_active == 0) {
	    setconnmode(0);
	}
#else	/* defined(TN3270) */
	setconnmode(0);
#endif	/* defined(TN3270) */
    }
}

/*
 * Help command.
 */
	static
help(argc, argv)
	int argc;
	char *argv[];
{
	register Command *c;

	if (argc == 1) {
		pfmt(stdout, MM_NOSTD, ":341:Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c->name; c++)
			if (c->help) {
				pfmt(stdout, MM_NOSTD, ":342:%-*s\t", HELPINDENT, c->name);
				pfmt(stdout, MM_NOSTD, c->help);
				pfmt(stdout, MM_NOSTD, ":39:\n");
			}
		return 0;
	}
	while (--argc > 0) {
		register char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (Ambiguous(c))
			pfmt(stdout, MM_NOSTD, ":343:?Ambiguous help command %s\n", arg);
		else if (c == (Command *)0)
			pfmt(stdout, MM_NOSTD, ":344:?Invalid help command %s\n", arg);
		else {
			pfmt(stdout, MM_NOSTD, c->help);
			pfmt(stdout, MM_NOSTD, ":39:\n");
		}
	}
	return 0;
}

static char *rcname = 0;
static char rcbuf[128];

cmdrc(m1, m2)
	char *m1, *m2;
{
    register Command *c;
    FILE *rcfile;
    int gotmachine = 0;
    int l1 = strlen(m1);
    int l2 = strlen(m2);
    char m1save[64];

    if (skiprc)
	return;

    strcpy(m1save, m1);
    m1 = m1save;

    if (rcname == 0) {
	rcname = getenv("HOME");
	if (rcname)
	    strcpy(rcbuf, rcname);
	else
	    rcbuf[0] = '\0';
	strcat(rcbuf, "/.telnetrc");
	rcname = rcbuf;
    }

    if ((rcfile = fopen(rcname, "r")) == 0) {
	return;
    }

    for (;;) {
	if (fgets(line, sizeof(line), rcfile) == NULL)
	    break;
	if (line[0] == 0)
	    break;
	if (line[0] == '#')
	    continue;
	if (gotmachine) {
	    if (!isspace(line[0]))
		gotmachine = 0;
	}
	if (gotmachine == 0) {
	    if (isspace(line[0]))
		continue;
	    if (strncasecmp(line, m1, l1) == 0)
		strncpy(line, &line[l1], sizeof(line) - l1);
	    else if (strncasecmp(line, m2, l2) == 0)
		strncpy(line, &line[l2], sizeof(line) - l2);
	    else if (strncasecmp(line, "DEFAULT", 7) == 0)
		strncpy(line, &line[7], sizeof(line) - 7);
	    else
		continue;
	    if (line[0] != ' ' && line[0] != '\t' && line[0] != '\n')
		continue;
	    gotmachine = 1;
	}
	makeargv();
	if (margv[0] == 0)
	    continue;
	c = getcmd(margv[0]);
	if (Ambiguous(c)) {
	    pfmt(stdout, MM_NOSTD, ":345:?Ambiguous command: %s\n", margv[0]);
	    continue;
	}
	if (c == 0) {
	    pfmt(stdout, MM_NOSTD, ":346:?Invalid command: %s\n", margv[0]);
	    continue;
	}
	/*
	 * This should never happen...
	 */
	if (c->needconnect && !connected) {
	    pfmt(stdout, MM_NOSTD, ":347:?Need to be connected first for %s.\n", margv[0]);
	    continue;
	}
	(*c->handler)(margc, margv);
    }
    fclose(rcfile);
}

#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)

/*
 * Source route is handed in as
 *	[!]@hop1@hop2...[@|:]dst
 * If the leading ! is present, it is a
 * strict source route, otherwise it is
 * assmed to be a loose source route.
 *
 * We fill in the source route option as
 *	hop1,hop2,hop3...dest
 * and return a pointer to hop1, which will
 * be the address to connect() to.
 *
 * Arguments:
 *	arg:	pointer to route list to decipher
 *
 *	cpp: 	If *cpp is not equal to NULL, this is a
 *		pointer to a pointer to a character array
 *		that should be filled in with the option.
 *
 *	lenp:	pointer to an integer that contains the
 *		length of *cpp if *cpp != NULL.
 *
 * Return values:
 *
 *	Returns the address of the host to connect to.  If the
 *	return value is -1, there was a syntax error in the
 *	option, either unknown characters, or too many hosts.
 *	If the return value is 0, one of the hostnames in the
 *	path is unknown, and *cpp is set to point to the bad
 *	hostname.
 *
 *	*cpp:	If *cpp was equal to NULL, it will be filled
 *		in with a pointer to our static area that has
 *		the option filled in.  This will be 32bit aligned.
 * 
 *	*lenp:	This will be filled in with how long the option
 *		pointed to by *cpp is.
 *	
 */
	unsigned long
sourceroute(arg, cpp, lenp)
	char	*arg;
	char	**cpp;
	int	*lenp;
{
	static char lsr[44];
	char *cp, *cp2, *lsrp, *lsrep;
	register int tmp;
	struct in_addr sin_addr;
	register struct hostent *host = 0;
	register char c;

	/*
	 * Verify the arguments, and make sure we have
	 * at least 7 bytes for the option.
	 */
	if (cpp == NULL || lenp == NULL)
		return((unsigned long)-1);
	if (*cpp != NULL && *lenp < 7)
		return((unsigned long)-1);
	/*
	 * Decide whether we have a buffer passed to us,
	 * or if we need to use our own static buffer.
	 */
	if (*cpp) {
		lsrp = *cpp;
		lsrep = lsrp + *lenp;
	} else {
		*cpp = lsrp = lsr;
		lsrep = lsrp + 44;
	}

	cp = arg;

	/*
	 * Next, decide whether we have a loose source
	 * route or a strict source route, and fill in
	 * the begining of the option.
	 */
	if (*cp == '!') {
		cp++;
		*lsrp++ = IPOPT_SSRR;
	} else
		*lsrp++ = IPOPT_LSRR;

	if (*cp != '@')
		return((unsigned long)-1);

	lsrp++;		/* skip over length, we'll fill it in later */
	*lsrp++ = 4;

	cp++;

	sin_addr.s_addr = 0;

	for (c = 0;;) {
		if (c == ':')
			cp2 = 0;
		else for (cp2 = cp; c = *cp2; cp2++) {
			if (c == ',') {
				*cp2++ = '\0';
				if (*cp2 == '@')
					cp2++;
			} else if (c == '@') {
				*cp2++ = '\0';
			} else if (c == ':') {
				*cp2++ = '\0';
			} else
				continue;
			break;
		}
		if (!c)
			cp2 = 0;

		if ((tmp = inet_addr(cp)) != -1) {
			sin_addr.s_addr = tmp;
		} else if (host = gethostbyname(cp)) {
#if	defined(h_addr)
			memcpy((caddr_t)&sin_addr,
				host->h_addr_list[0], host->h_length);
#else
			memcpy((caddr_t)&sin_addr, host->h_addr, host->h_length);
#endif
		} else {
			*cpp = cp;
			return(0);
		}
		memcpy(lsrp, (char *)&sin_addr, 4);
		lsrp += 4;
		if (cp2)
			cp = cp2;
		else
			break;
		/*
		 * Check to make sure there is space for next address
		 */
		if (lsrp + 4 > lsrep)
			return((unsigned long)-1);
	}
	if ((*(*cpp+IPOPT_OLEN) = lsrp - *cpp) <= 7) {
		*cpp = 0;
		*lenp = 0;
		return((unsigned long)-1);
	}
	*lsrp++ = IPOPT_NOP; /* 32 bit word align it */
	*lenp = lsrp - *cpp;
	return(sin_addr.s_addr);
}
#endif

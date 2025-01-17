/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/error.c	1.9.14.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/error.c,v 1.1 91/02/28 20:08:23 ccs Exp $"
/*
 * UNIX shell
 */

#include	<pfmt.h>
#include	<string.h>
#include	"defs.h"

static void com_failed();

extern int unlink();

/* ========	error handling	======== */

void
failed(cmd, s1, s2, s2id)
int cmd;
const char unsigned *s1;
const char	 *s2, *s2id;
{
	com_failed(cmd, MM_ERROR, s1, s2, s2id);
	exitsh(ERROR);
}

void
error(cmd, s, sid)
int cmd;
const char	*s, *sid;
{
	failed(cmd, (const unsigned char *)gettxt(sid, s), NIL, NIL);
}

void
error_fail(cmd, s, sid)
int cmd;
const char	*s, *sid;
{
	failure(cmd, (const unsigned char *)gettxt(sid, s), NIL, NIL);
}

void
warning(cmd, s, sid)
int cmd;
const char	*s, *sid;
{
	com_failed(cmd, MM_WARNING, (const unsigned char *)gettxt(sid, s), NIL, NIL);
}

void
exitsh(xno)
int	xno;
{
	/*
	 * Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval = xno;
	flags |= eflag;
	if ((flags & (forcexit | forked | errflg | ttyflg)) != ttyflg)
		done(0);
	else
	{
		clearup();
		restore(0);
		(void)setb(1);
		execbrk = breakcnt = funcnt = 0;
		longjmp(errshell, 1);
	}
}

void
rmtemp(base)
struct ionod	*base;
{
	while (iotemp > base)
	{
		(void)unlink(iotemp->ioname);
		free(iotemp->iolink);
		iotemp = iotemp->iolst;
	}
}

void
rmfunctmp()
{
	while (fiotemp)
	{
		(void)unlink(fiotemp->ioname);
		fiotemp = fiotemp->iolst;
	}
}

void
failure(cmd, s1, s2, s2id)
int cmd;
const char unsigned *s1;
const char	 *s2, *s2id;
{
	com_failed(cmd, MM_ERROR, s1, s2, s2id);
	if (flags & errflg)
		exitsh(ERROR);

	flags |= eflag;
	exitval = ERROR;
	exitset();
}

void
prusage(cmd, s, sid)
int cmd;
const char *s, *sid;
{
	flushb();
	(void)set_label(cmd);

	pfmt(stderr, MM_ERROR, badusage);
	pfmt(stderr, MM_ACTION, usage, gettxt(sid, s));

	if (flags & errflg)
		exitsh(ERROR);

	flags |= eflag;
	exitval = ERROR;
	exitset();
}

void
pr_usage(cmd, s, sid)
int cmd;
const char *s, *sid;
{
	flushb();
	(void)set_label(cmd);

	pfmt(stderr, MM_ERROR, badusage);
	pfmt(stderr, MM_ACTION, usage, gettxt(sid, s));
	exitsh(ERROR);
}

static void
com_failed(cmd, flag, s1, s2, s2id)
int cmd;
long flag;
const char unsigned *s1;
const char	 *s2, *s2id;
{
	int comb_label;
	flushb();
	comb_label = set_label(cmd);
	pfmt(stderr, flag, NULL);

	if (!comb_label)
		prp();
	
	if (s2)
		prs_cntl(s1);
	else
		prs(s1);

	if (s2)
	{
		prs(gettxt(colonid, colon));
		if (s2id)
			prs(gettxt(s2id, s2));
		else
			prs(s2);
	}
	newline();
}

int
set_label(cmd)
int cmd;
{
	char *cmd_lbl = "UX:sh";
	char cmd_buf[MAXLABEL];
	int i;
	int retcode = 0;
	
	switch(cmd){
	case 0:
		break;
	case SYSFG:
		cmd_lbl = "UX:fg";
		break;
	case SYSBG:
		cmd_lbl = "UX:bg";
		break;
	case SYSTST:
		cmd_lbl = "UX:test";
		break;
	default:
		for (i = 0 ; i < no_commands ; i++){
			if (commands[i].sysval == cmd)
				break;
		}
		if (i != no_commands){
			(void)strcpy(cmd_buf, "UX:");
			(void)strcpy(cmd_buf + 3, commands[i].sysnam);
			cmd_lbl = cmd_buf;
		}
	}
	if ((flags & prompt) == 0 && cmdadr){
		/* Create a combined label */
		int len, len2;
		char *ptr;

		len = strlen(cmd_lbl) + 3; /* For space and () */
		len2 = strlen((char *)cmdadr);
		if (len + len2 >= MAXLABEL && (ptr = strrchr((char *)cmdadr, '/')) != 0){
			ptr++;
			len2 = strlen(ptr);
			if (len + len2 >= MAXLABEL)
				ptr = 0;
		} else
			ptr = (char *)cmdadr;
		if (ptr){
			if (cmd_lbl != cmd_buf){
				(void)strcpy(cmd_buf, cmd_lbl);
				cmd_lbl = cmd_buf;
			}
			(void)strcpy(cmd_buf + len - 3, " (");
			(void)strcpy(cmd_buf + len - 1, ptr);
			(void)strcpy(cmd_buf + len - 1 + len2, ")");
			retcode = 1;
		}
	} else
		retcode = 1;
	
	(void)setlabel(cmd_lbl);
	return retcode;
}

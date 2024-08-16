/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/process.c	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)process.c	1.2 'attmail mail(1) command'"
#include	"../mail/libmail.h"
#include "mailproc.h"

int		Prtdomain = 0;		/* use domain style address? */
char		Quoted = '\0';		/* is this string to be quoted? */

static char	*Msgfile = NULL;
static char	*Txtfile = NULL;
static char	*Hdrfile = NULL;

static char *gettmp ARGS((int));


/* SCCSid = @(#)cmd.l	3.1 */
/* source SCCSid = @(#)cmd.l	2.7 */
Cmd	*Cmdptr = NULL;
static char	*Cmdline = NULL;

/*
 * input and unput functions required by yacc/lex
 */
int input()
{
	if (!Cmdptr || !*Cmdline)
		return 0;

	return *Cmdline++;
}

int unput(c)
int c;
{
	if (Cmdptr && Cmdline && (Cmdline > Cmdptr->cmdbuf))
		*--Cmdline = (char)c;
	return c;
}

yywrap()
{
	return 1;
}

/* process a message against the list of commands */
int process(mp)
Msg	*mp;
{
	int	ret;

	if (mp->processed)
		return 1;

	mp->processed++;

	if (dflg)
	{
		(void) pfmt(stderr, MM_NOSTD, ":588:\n======== Message: ========\n");
		(void) fprintf(stderr, "Subj:\t%s\nDate:\t%s\nFrom:\t",
				mp->subject, mp->date);
		prtfrom(stderr, mp->fhp, 1);
		(void) fputc('\n', stderr);
	}

	/*
	 * process message with every command until an error
	 * found or EOF found or we get a good match.
	 */
	Prtdomain = mp->fhp->domain;
	Msgptr = mp;
	ret = 1;
	for(Cmdptr = Cmdlist; Cmdptr; Cmdptr = Cmdptr->next)
	{
		Quoted = '\0';			/* initialize quote watch */
		Cmdline = Cmdptr->cmdbuf;
		switch(yyparse())
		{
			case P_MATCH:		/* good match */
				ret = 1;
				break;
			case P_NOMATCH:		/* no match */
				continue;
			case P_ERROR:		/* syntax error */
				(void) pfmt(stderr, MM_ERROR, ":589:Syntax error: %s\n", Cmdptr->cmdbuf);
				ret = 0;
				break;
			case P_ERR_FROM:	/* error parsing from */
				(void) pfmt(stderr, MM_ERROR, ":590:error parsing <from> portion: %s\n", Cmdptr->cmdbuf);
				ret = 0;
				break;
			case P_ERR_MEM:		/* ran out of memory */
				(void) pfmt(stderr, MM_ERROR, ":382:Problem allocating memory\n");
				ret = 0;
				break;
			case P_ERR_TSYS:	/* error in test sh command */
				(void) pfmt(stderr, MM_ERROR, ":591:shell error in <test>: %s\n", Cmdptr->cmdbuf);
				ret = 0;
				break;
			case P_ERR_ASYS:	/* error in action sh cmd */
				(void) pfmt(stderr, MM_ERROR, ":592:shell error in <action>: %s\n", Cmdptr->cmdbuf);
				ret = 0;
				break;
			case P_EOF:		/* EOF */
				ret = 1;
				break;
		}

		break;
	}

	if (Msgfile)
	{
		if (dflg)
			(void) pfmt(stderr, MM_INFO, ":593:Unlinking file %s\n", Msgfile);
		(void) unlink(Msgfile);
		Msgfile = NULL;
	}
	if (Txtfile)
	{
		if (dflg)
			(void) pfmt(stderr, MM_INFO, ":593:Unlinking file %s\n", Txtfile);
		(void) unlink(Txtfile);
		Txtfile = NULL;
	}
	if (Hdrfile)
	{
		if (dflg)
			(void) pfmt(stderr, MM_INFO, ":593:Unlinking file %s\n", Hdrfile);
		(void) unlink(Hdrfile);
		Hdrfile = NULL;
	}

	return ret;
}

/*
 * setfile() sets the message or text file and places the
 * file name in s.
 */
int setfile(s, type)
char *s;
int type;
{
	if (type == M_MSG)
	{
		/* message into file */
		if (!Msgfile)
		{
			Msgfile = gettmp(M_MSG);
			if (!mvmsg(Msgptr, Msgfile, M_MSG, M_DIR, 0))
				return 0;
		}
		(void) strcpy(s, Msgfile);
	}
	else if (type == M_TXT)
	{
		/* message into file */
		if (!Txtfile)
		{
			Txtfile = gettmp(M_TXT);
			if (!mvmsg(Msgptr, Txtfile, M_TXT, M_DIR, 0))
				return 0;
		}
		(void) strcpy(s, Txtfile);
	}
	else
	{
		/* message into file */
		if (!Hdrfile)
		{
			Hdrfile = gettmp(M_HDR);
			if (!mvmsg(Msgptr, Hdrfile, M_HDR, M_DIR, 0))
				return 0;
		}
		(void) strcpy(s, Hdrfile);
	}

	return 1;
}

/*
 * gettmp returns a file name in the /usr/tmp directory
 */
static char *gettmp(type)
int type;
{
	static	char	Msgbuf[512] = "";
	static	char	Txtbuf[512] = "";
	static	char	Hdrbuf[512] = "";
	char		*bp;

	switch(type)
	{
		case M_MSG:		/* full message */
			bp = Msgbuf;
			break;

		case M_TXT:		/* message text */
			bp = Txtbuf;
			break;

		case M_HDR:		/* message header */
			bp = Hdrbuf;
			break;
	}

	if (*bp)
		return bp;

	(void) tmpnam(bp);
	return bp;
}

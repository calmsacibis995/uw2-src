/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/parse.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)parse.c	1.1 'attmail mail(1) command'"
/* from ferret SCCSid parse.c 3.1 */
#include	"../mail/libmail.h"
#include "mailproc.h"

Msg *msglist = NULL;

static int header ARGS((char*));
static char *shortfrom ARGS((Msg*, int));	/* short form of from line */

/*
 * parsemail(infp, outp)
 * routine to parse a mail file.  Looks for From information line
 * of the form:
 *	[>]From name date [ { remote from | forwarded by } name ]
 *	[ Subject: <subject> ]
 *	[ Status: <status> ]
 *	[ Content-Length: length ]
 *	blank line
 *	[ body ]
 */
int parsemail(infp, outfp)
FILE *infp;
FILE *outfp;
{
	int	in_msg;			/* in a message? */
	int	msgno;			/* message number */
	int	new_found;		/* new message found */
	char	buf[BUFSIZ];		/* buffer size */
	long	pos;			/* position in file */
	Msg	*last;			/* message pointer */
	Msg	*msg;			/* message pointer */
	long	clen;			/* value of Content-Length: header */
	FILE	*nullfp = 0;		/* /dev/null */

	new_found = 0;
	in_msg = 1;
	msgno = 0;
	msg = NULL;
	clen = 0;
	if (lflg) nullfp = fopen("/dev/null", "w");
	for(last = msglist; last && last->next; last = last->next)
		;
	while(pos = ftell(lflg?infp:outfp), fgets(buf, sizeof(buf), infp))
	{
		if (!lflg)
			(void) fputs(buf, outfp);

		if (pflg && msg && in_msg)
			continue;

		if (in_msg)
		{
			/* check to see if at the beginning of next msg */
			if (strncmp(buf, "From ", 5) != 0)
				continue;

			in_msg = 0;
			if (msg)
			{
				/* tail of previous message */
				msg->msize = (int)(pos - msg->startmsg);
				msg->tsize = (int)(pos - msg->starttxt);
			}

			/* create structure for new message */
			if ((msg = new_Msg(!aflg, pos)) == NULL)
			{
				if (nullfp) fclose(nullfp);
				return -1;
			}

			if (!last)
				msglist = msg;
			else
				last->next = msg;
			last = msg;

			parsefrom(&buf[5], msg);
			clen = 0;
			continue;
		}

		/* process message header */
		if (strncmp(buf, ">From ", 6) == 0)
			parsefrom(&buf[6], msg);
		else if (casncmp(buf, "Subject:", 8) == 0)
		{
			buf[strlen(buf)-1] = '\0';
			(void) strncpy(msg->subject, skipspace(buf+8), sizeof(msg->subject)-1);
			msg->subject[sizeof(msg->subject)-1] = '\0';
		}
		else if (casncmp(buf, "Status:", 7) == 0)
		{
			buf[strlen(buf)-1] = '\0';
			(void) strncpy(msg->status, skipspace(buf+7),sizeof(msg->status)-1);
			msg->status[sizeof(msg->status)-1] = '\0';
			if (aflg)
				new_found++;
		}
		else if (casncmp(buf, "Content-Length:", 15) == 0)
		{
			if (clen == 0)
				clen = atoi(buf + 15);
		}
		else if (!header(buf))
		{
			if (!*(msg->status))
			{
				msg->processed = 0;
				new_found++;
				if (lflg)
				{
					*msg->status = 'N';
					*(msg->status+1) = '\0';
				}
			}
			in_msg = 1;
			msgno++;
			msg->starttxt = pos;
			if (lflg && !msg->processed)
			{
				(void) printf("%2d: %-2.2s %-15s %16.16s %.37s\n",
					msgno, msg->status, shortfrom(msg, 15), msg->date, msg->subject);
			}
			if (clen > 0)
			{
				if (lflg) copynstream(infp, nullfp, clen);
				else copynstream(infp, outfp, clen);
			}
		}

	}

	if (msg)
	{
		/* tail of previous message */
		msg->msize = (int)(pos - msg->startmsg);
		msg->tsize = (int)(pos - msg->starttxt);
	}

	if (nullfp) fclose(nullfp);
	return new_found;
}

/* look for an RFC822-style header */
static int header(bp)
register char *bp;
{
	/* look for continuation line */
	if ((*bp == ' ') || (*bp == '\t'))
		return 1;

	/* look for : */
	for ( ; *bp ; bp++)
	{
		if (Iscntrl(*bp) || Isspace(*bp))
			return 0;
		if (*bp == ':')
			return 1;
	}
	return 0;
}

static char *shortfrom(mp, len)
Msg *mp;
int len;
{
	static	char	shortname[100];

	if (mp->fhp->first)
	{
		if (strlen(mp->fhp->user)+strlen(mp->fhp->first->hostname) <= len)
		{
			if (mp->fhp->domain)
				(void) sprintf(shortname, "%s@%s", mp->fhp->user, mp->fhp->first->hostname);
			else
				(void) sprintf(shortname, "%s!%s", mp->fhp->first->hostname, mp->fhp->user);

			return shortname;
		}
	}
	return mp->fhp->user;
}

/* allocate and initialize a new Msg structure */
struct Msg *new_Msg(processed_flag, pos)
int processed_flag;
int pos;
{
	Msg *msg = (Msg *)malloc(sizeof(Msg));
	if (msg == NULL)
		return 0;
	if ((msg->fhp = new_Fromlist()) == NULL)
	{
		free(msg);
		return 0;
	}

	msg->next = NULL;
	msg->processed = processed_flag;
	msg->keep = 1;
	msg->startmsg = pos;
	msg->status[0] = '\0';
	msg->subject[0] = '\0';
	return msg;
}

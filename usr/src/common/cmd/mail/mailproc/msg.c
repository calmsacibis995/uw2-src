/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/msg.c	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)msg.c	1.2 'attmail mail(1) command'"
/* from ferret SCCSid msg.c 3.1 */
#include "../mail/libmail.h"
#include "mailproc.h"

static	int	movemsg ARGS((int, int, long, int));		/* message mover */

/*
 * Routines to move messages around.
 * mvmsg() writes the given piece of a message to the given file
 */
int mvmsg(mp, to, part, mode, nl)
Msg *mp;				/* message pointer */
char *to;				/* destination file name */
int part;				/* M_TXT (text) M_MSG (msg) */
int mode;				/* M_DIR (>file) M_APP (>>file) */
int nl;					/* append a carriage return to msg? */
{
	int		fd;		/* file descriptor */
	int		omode;		/* open file mode */
	int		ret;		/* return value */
	int		size;		/* size of message */
	char		*p;		/* char pointer */
	long		offset;		/* offset into msg file */

	to += strspn(to, " \t");
	if (p = strpbrk(to, " \t\n"))
		*p = '\0';

	omode = O_WRONLY|O_CREAT|(mode == M_APP ? O_APPEND : O_TRUNC);
	if ((fd = open(to, omode, 0600)) < 0)
		return 0;

	if (part == M_TXT)
	{
		offset = mp->starttxt;
		size = mp->tsize;
	}
	else if (part == M_MSG)
	{
		offset = mp->startmsg;
		size = mp->msize;
	}
	else	/* part == M_HDR */
	{
		offset = mp->startmsg;
		size = mp->msize - mp->tsize;
	}

	if (dflg)
		(void)pfmt(stderr, MM_INFO,
		    (mode == M_APP) ?
		        ((part == M_TXT) ?	":582:Appending the text to file %s (%ld:%d)\n" :
			 (part == M_MSG) ? 	":583:Appending the message to file %s (%ld:%d)\n" :
			 /* (part == M_HDR) */	":584:Appending the header to file %s (%ld:%d)\n") :
		    /* (mode == M_DIR) */
		    	((part == M_TXT) ?	":585:Writing the text to file %s (%ld:%d)\n" :
			 (part == M_MSG) ?	":586:Writing the message to file %s (%ld:%d)\n" :
			 /* (part == M_HDR) */	":587:Writing the header to file %s (%ld:%d)\n"),
			to, offset, size);

	ret = movemsg(fileno(Mailfp), fd, offset, size);
	if (ret && nl)
		(void) write(fd, "\n", 1);	/* append new line */
	(void) close(fd);

	return ret;
}

/*
    write piece of a message to given file descriptor at the given location
*/
static int movemsg(mfd, fd, offset, size)
int mfd;
int fd;					/* file descriptor */
long offset;				/* offset into file */
int size;				/* size of text */
{
	int		ret;
	int		rdsize;
	char		buf[BUFSIZ];
	/*
	 * read mfd at offset 'offset' for 'size' bytes'
	 * and write result to fd.
	 */
	if (lseek(mfd, offset, 0) < 0)
		return 0;
	while(size)
	{
		rdsize = (size > sizeof(buf) ? sizeof(buf) : size);

		if ((ret = read(mfd, buf, rdsize)) < 0)
			return 0;

		if (write(fd, buf, ret) < 0)
			return 0;

		size -= rdsize;
	}

	return 1;
}

/*
 * copyback copies messages from 'from' to 'to'
 */
int copyback(from, to, mp)
int from;
int to;
Msg *mp;
{
	for(; mp; mp = mp->next)
	{
		if (!mp->keep)
			continue;

		if (!*mp->status)
		{
			/* mark as mailproced */
			if ((movemsg(from, to, mp->startmsg, (mp->starttxt - mp->startmsg)) < 0) ||
			    (write(to, "Status: F\n", 10) < 0) ||
			    (movemsg(from,to,mp->starttxt, mp->tsize) < 0))
				return 0;
		}
		else
			if (movemsg(from, to, mp->startmsg, mp->msize) < 0)
				return 0;
	}

	return 1;
}

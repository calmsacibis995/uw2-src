/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/msgs/mdisconnect.c	1.6.1.3"
#ident	"$Header: $"
/* LINTLIBRARY */

# include	<stropts.h>
# include	<errno.h>
# include	<stdlib.h>

#include "lp.h"
#include "msgs.h"

#if	defined(__STDC__)
static void	disconnect3_2 ( MESG * );
#else
static void	disconnect3_2();
#endif

#if	defined(__STDC__)
int mdisconnect ( MESG * md )
#else
int mdisconnect (md)
    MESG	*md;
#endif
{
    int		retvalue = 0;
    void	(**fnp)();
    MQUE	*p;
    
    if (!md)
    {
	errno = ENXIO;
	return(-1);
    }

    switch(md->type)
    {
	case MD_CHILD:
	case MD_STREAM:
	case MD_BOUND:
	    if (md->writefd >= 0)
		(void) Close(md->writefd);
	    if (md->readfd >= 0)
		(void) Close(md->readfd);
	    break;

	case MD_USR_FIFO:
	case MD_SYS_FIFO:
	   disconnect3_2(md);
	   break;
    }

    if (md->on_discon)
    {
	for (fnp = md->on_discon; *fnp; fnp++)
	{
	    (*fnp)(md);
	    retvalue++;
	}
	Free(md->on_discon);
    }

    if (md->file)
	Free(md->file);
    
    if (md->mque)
    {
	while ((p = md->mque) != NULL)
	{
	    md->mque = p->next;
	    Free(p->dat->buf);
	    Free(p->dat);
	    Free(p);
	}
    }
    if (md->credp)
	Free (md->credp);
    Free(md);

    return(retvalue);
}

int	discon3_2_is_running = 0;

#if	defined(__STDC__)
static void disconnect3_2 ( MESG * md )
#else
static void disconnect3_2 (md)
    MESG	*md;
#endif
{
    char	*msgbuf = 0;
    int		size;

    discon3_2_is_running = 1;

    if (md->writefd != -1)
    {
	size = putmessage((char *)0, S_GOODBYE);
	if ((msgbuf = (char *)Malloc((unsigned)size)))
	{
	    (void)putmessage (msgbuf, S_GOODBYE);
	    (void)msend (msgbuf);
	    Free (msgbuf);
	}

	(void) Close (md->writefd);
    }

    if (md->readfd != -1)
	(void) Close (md->readfd);

    if (md->file)
    {
	(void) Unlink (md->file);
	Free (md->file);
    }	

    discon3_2_is_running = 0;
}

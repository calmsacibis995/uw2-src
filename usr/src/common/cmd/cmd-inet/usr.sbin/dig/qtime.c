/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/qtime.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
** Written with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

#include <sys/types.h>
#include "qtime.h"

static int qptr = 0;
struct timelist _qtime[HMAXQTIME];
struct timeval hqtime;
u_short hqcount, hxcount;

struct timeval
*findtime(id)
     u_short id;
{
int i;
  for (i=0; i<HMAXQTIME; i++)
    if (_qtime[i].id == id)
      return(&(_qtime[i].time));
  return(NULL);
}


savetime(id,t)
     u_short id;
     struct timeval *t;
{
qptr = ++qptr % HMAXQTIME;
_qtime[qptr].id = id;
_qtime[qptr].time.tv_sec = t->tv_sec;
_qtime[qptr].time.tv_usec = t->tv_usec;
}


struct timeval
*difftv(a,b,tmp)
     struct timeval *a, *b, *tmp;
{
  tmp->tv_sec = a->tv_sec - b->tv_sec;
  if ((tmp->tv_usec = a->tv_usec - b->tv_usec) < 0) {
    tmp->tv_sec--;
    tmp->tv_usec += 1000000;
  }
return(tmp);
}


prnttime(t)
     struct timeval *t;
{
printf("%u msec ",t->tv_sec * 1000 + (t->tv_usec / 1000));
}


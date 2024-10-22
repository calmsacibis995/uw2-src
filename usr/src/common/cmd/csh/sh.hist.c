/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/sh.hist.c	1.3.5.4"
#ident  "$Header: sh.hist.c 1.2 91/06/26 $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "sh.h"
#include "sh.tconst.h"

/*
 * C shell
 */

savehist(sp)
	struct wordent *sp;
{
	register struct Hist *hp, *np;
	register int histlen = 0;
	tchar *cp;

#ifdef TRACE
	tprintf("TRACE- savehist()\n");
#endif
	/* throw away null lines */
	if (sp->next->word[0] == '\n')
		return;
	cp = value(S_history /*"history"*/);
	if (*cp) {
		register tchar *p = cp;

		while (*p) {
			if (!digit(*p)) {
				histlen = 0;
				break;
			}
			histlen = histlen * 10 + *p++ - '0';
		}
	}
	for (hp = &Histlist; np = hp->Hnext;)
		if (eventno - np->Href >= histlen || histlen == 0)
			hp->Hnext = np->Hnext, hfree(np);
		else
			hp = np;
	(void) enthist(++eventno, sp, 1);
}

struct Hist *
enthist(event, lp, docopy)
	int event;
	register struct wordent *lp;
	bool docopy;
{
	register struct Hist *np;

#ifdef TRACE
	tprintf("TRACE- enthist()\n");
#endif
	np = (struct Hist *) xalloc(sizeof *np);
	np->Hnum = np->Href = event;
	if (docopy)
		copylex(&np->Hlex, lp);
	else {
		np->Hlex.next = lp->next;
		lp->next->prev = &np->Hlex;
		np->Hlex.prev = lp->prev;
		lp->prev->next = &np->Hlex;
	}
	np->Hnext = Histlist.Hnext;
	Histlist.Hnext = np;
	return (np);
}

hfree(hp)
	register struct Hist *hp;
{
#ifdef TRACE
	tprintf("TRACE- hfree()\n");
#endif

	freelex(&hp->Hlex);
	xfree( (tchar *)hp);
}

dohist(vp)
	tchar **vp;
{
	int n, rflg = 0, hflg = 0;
#ifdef TRACE
	tprintf("TRACE- dohist()\n");
#endif
	if (getn(value(S_history /*"history"*/)) == 0)
		return;
	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
	while (*++vp && **vp == '-') {
		tchar *vp2 = *vp;

		while (*++vp2)
			switch (*vp2) {
			case 'h':
				hflg++;
				break;
			case 'r':
				rflg++;
				break;
			case '-':	/* ignore multiple '-'s */
				break;
			default:
				SFLAG_E,printf(gettxt(ER_UNKNOWN_F), *vp2);
				SFLAG_A,error(gettxt(USG_HISTORY));
			}
	}
	if (*vp)
		n = getn(*vp);
	else {
		n = getn(value(S_history /*"history"*/));
	}
	dohist1(Histlist.Hnext, &n, rflg, hflg);
}

dohist1(hp, np, rflg, hflg)
	struct Hist *hp;
	int *np, rflg, hflg;
{
	bool print = (*np) > 0;
#ifdef TRACE
	tprintf("TRACE- dohist1()\n");
#endif
top:
	if (hp == 0)
		return;
	(*np)--;
	hp->Href++;
	if (rflg == 0) {
		dohist1(hp->Hnext, np, rflg, hflg);
		if (print)
			phist(hp, hflg);
		return;
	}
	if (*np >= 0)
		phist(hp, hflg);
	hp = hp->Hnext;
	goto top;
}

phist(hp, hflg)
	register struct Hist *hp;
	int hflg;
{
#ifdef TRACE
	tprintf("TRACE- phist()\n");
#endif

	if (hflg == 0)
		printf("%6d\t", hp->Hnum);
	prlex(&hp->Hlex);
}

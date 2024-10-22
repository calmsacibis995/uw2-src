/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/eptstat.c	1.4.7.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkginfo.h>
#include <limits.h>

int	otherstoo;
int	usedbycore;
int	exist_pkginst;
char	*useclass;

#define ERR_MEMORY	"uxpkgtools:6:memory allocation failure, errno=%d"
extern int	errno;

extern void	*calloc();
extern void	progerr(),
		quit(),
		free();

struct pinfo *
eptstat(entry, pkg, c)
struct cfent *entry;
char	*pkg;
char	c;
{
	struct pinfo *pinfo, *last, *me, *myparent;

	exist_pkginst = usedbycore = otherstoo = 0;
	useclass = entry->class;

	me = myparent = last = (struct pinfo *)0;
	for(pinfo=entry->pinfo; pinfo; pinfo=pinfo->next) {
		if(!strcmp(pkg, pinfo->pkg)) {
			if(*pinfo->aclass)
				useclass = pinfo->aclass;
			myparent = last;
			me = pinfo;
			exist_pkginst++;
		} else
			otherstoo++;
		if(!strcmp(COREPKG, pinfo->pkg))
			usedbycore++;
		last = pinfo;
	}

	if(c) {
		/* use a delete/add strategy to keep package list
		 * ordered by modification time
		 */
		if(me) {
			/* remove from list first */
			if(myparent)
				myparent->next = me->next;
			else
				entry->pinfo = me->next;
			if(me == last)
				last = myparent;
			entry->npkgs--;
			/* leave 'me' around until later! */
		}
		if((c != '@') && (me || (c != '-'))) {
			/* need to add onto end */
			entry->npkgs++;
			if(me == NULL) {
				me = (struct pinfo *)calloc(1, 
					sizeof(struct pinfo));
				if(me == NULL) {
					progerr(ERR_MEMORY, errno);
					quit(99);
				}
			} else {
				me->next = (struct pinfo *)NULL;
				if(entry->npkgs == 1) {
					if(me->aclass[0])
						(void) strcpy(entry->class, 
							me->aclass);
					useclass = entry->class;
				} else
					useclass = me->aclass;
			}
			(void) strncpy(me->pkg, pkg, PKGSIZ);
			me->status = ((c == '#') ? '\0' : c);
			if(last)
				last->next = me; /* add to end */
			else
				entry->pinfo = me; /* only item */
		} else {
			/* just wanted to remove this package from list */
			if(me) {
				free(me);
				me = (struct pinfo *) 0;
			}
		}
	}
	return(me);
}

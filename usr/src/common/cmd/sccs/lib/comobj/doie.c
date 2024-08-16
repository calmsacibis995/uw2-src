/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/doie.c	6.4.1.1"
# include	"../../hdr/defines.h"
# include <pfmt.h>
# include <locale.h>
# include <sys/euc.h>
# include <limits.h>

void
doie(pkt,ilist,elist,glist)
struct packet *pkt;
char *ilist, *elist, *glist;
{
	void	dolist();

	if (ilist) {
		if (pkt->p_verbose & DOLIST) {
			fprintf(pkt->p_stdout,"================\n");
			pfmt(pkt->p_stdout,MM_ERROR,":212:Included:\n");

			dolist(pkt,ilist,INCLUDE);
			fprintf(pkt->p_stdout,"================\n");
		}
		else dolist(pkt,ilist,INCLUDE);
	}
	if (elist) {
		if (pkt->p_verbose & DOLIST) {
			fprintf(pkt->p_stdout,"================\n");
			pfmt(pkt->p_stdout,MM_ERROR,":213:Excluded:\n");

			dolist(pkt,elist,EXCLUDE);
			fprintf(pkt->p_stdout,"================\n");
		}
		else dolist(pkt,elist,EXCLUDE);
	}
	if (glist)
		dolist(pkt,glist,IGNORE);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/sidtoser.c	6.3"
# include	"../../hdr/defines.h"

int
sidtoser(sp,pkt)
register struct sid *sp;
struct packet *pkt;
{
	register int n;
	register struct idel *rdp;

	for (n = maxser(pkt); n; n--) {
		rdp = &pkt->p_idel[n];
		if (rdp->i_sid.s_rel == sp->s_rel &&
			rdp->i_sid.s_lev == sp->s_lev &&
			rdp->i_sid.s_br == sp->s_br &&
			rdp->i_sid.s_seq == sp->s_seq)
				break;
	}
	return(n);
}

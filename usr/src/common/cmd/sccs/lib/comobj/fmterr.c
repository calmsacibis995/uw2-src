/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/fmterr.c	6.3.1.1"
# include	"../../hdr/defines.h"

void
fmterr(pkt)
register struct packet *pkt;
{
	int	fatal();
	(void) fclose(pkt->p_iop);
	fatal(
		":216:format error at line %d (co4)",pkt->p_slnno);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/eqsid.c	6.3"
# include	"../../hdr/defines.h"

int
eqsid(s1, s2)
register struct sid *s1, *s2;
{
	if (s1->s_rel == s2->s_rel &&
		s1->s_lev == s2->s_lev &&
		s1->s_br == s2->s_br &&
		s1->s_seq == s2->s_seq)
			return(1);
	else
		return(0);
}

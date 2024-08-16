/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/lddef.h	1.3"
#ifndef LDLIST

struct ldlist {
	LDFILE		ld_item;
	struct ldlist	*ld_next;
};

#define	LDLIST	struct ldlist
#define	LDLSZ	sizeof(LDLIST)

#endif

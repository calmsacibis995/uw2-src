/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/link.h	1.2"
/*
@(#)link.h	1.2 91/09/0310:29:24 91/09/0310:29:29
*/
/*
(C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or modified
in any manner or compiled, linked or uploaded or downloaded to or from any
computer system without the prior written consent of Novell, Inc.
*/
/*
	Author:	Scott Harrison
*/

#if	!defined(LINK_H)
#define	LINK_H

void
    *linkNew(void *),
    *linkInit(void *, void *),
    linkOwnerSet(void *, void *),
    linkFree(void *),
    *linkAddSorted(void *, void *, int (*)()),
    *linkAppend(void *, void *),
    *linkPrepend(void *, void *),
    *linkRemove(void *),
    *linkNext(void *),
    *linkLast(void *),
    *linkOwner(void *),
    *linkHead(void *);

int
    linkAddSortedUnique(void *, void *, int (*)(), void (*)(void *));

#endif

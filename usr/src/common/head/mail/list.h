/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/list.h	1.1"
#if	!defined(LIST_H)
#define	LIST_H

#if	!defined(LIST_OBJECT)
typedef void * list_t;
#endif

void
    listAdd(void *, char *),
    listAddWithFree(void *, char *, void (*)()),
    listFree(void *);

int 
    listGetNext(void *, char **);

list_t
    *listNew(void);

#endif

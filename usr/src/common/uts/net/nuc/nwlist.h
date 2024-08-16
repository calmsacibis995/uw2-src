/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwlist.h	1.9"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwlist.h,v 2.51.2.1 1994/12/12 01:26:54 stevbam Exp $"

#ifndef _UTIL_NUC_TOOLS_NUCT_NWLIST_H 
#define _UTIL_NUC_TOOLS_NUCT_NWLIST_H 

/*
 *  Netware Unix Client
 *
 *	MODULE:     list.h
 *	ABSTRACT:   Structures used in the generic linked list library
 */

/*
 *	Bit fields for the mode byte passed to InitList
 */
#define NWLIST_SEMAPHORE_BIT	0x01


/*
 *	Single linked list structures
 */
typedef struct slnode {
	struct slnode	*next;
	void_t			*data;
}SLNode;


typedef struct {
	SLNode	*head;
	SLNode	*tail;
	SLNode	*current;
	int32	useFlag;
}SLList;


/*
 *	Double linked list structures
 */

typedef struct dlnode {
	struct dlnode	*next;
	struct dlnode	*prev;
	void_t			*data;
}DLNode;

typedef struct {
	DLNode	*head;
	DLNode	*tail;
	DLNode	*current;
	int32	useFlag;
}DLList;

#endif /* _UTIL_NUC_TOOLS_NUCT_NWLIST_H */

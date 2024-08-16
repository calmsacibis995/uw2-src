/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/io/xtestqueue.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#ifndef _XTESTQ_H
#define _XTESTQ_H

/*
** Added for AT&T/i386 Server
*/

typedef struct XTestQueueNode
	{
		struct XTestQueueNode	*next;
		int			variant1;
		int			variant2;
		xqEvent			xqevent;
	} XTestQueueNode;

typedef struct
	{
		XTestQueueNode	*head;
		XTestQueueNode	*tail;
	} XTestQueue;

#define XTestQueueEmpty(q)	(!(q).head)
#define XTestQueueNotEmpty(q)	((q).head)

#endif

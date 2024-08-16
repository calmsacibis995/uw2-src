/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/notice.h	1.11"
#endif

#ifndef _NOTICE_H
#define _NOTICE_H

#define NUM_FIELDS 4

typedef struct NoticeItem {
	XtArgVal	flag;
	XtArgVal	function;
	XtArgVal	string;
	XtArgVal	mnemonic;
} NoticeItem;

typedef struct Notice {
	String		name;
	String		string;
	NoticeItem *	items;	/* of NoticeItem struct's */
	int		numitems;
	Widget		w;	/* holds the notice widget */
} Notice;

#endif

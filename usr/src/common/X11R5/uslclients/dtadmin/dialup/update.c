/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/update.c	1.11"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/FList.h>
#include "uucp.h"

extern void SetFields();
extern void ResetFields();

/* 
 * Do the following:
 * Select "new" in the scrolling list.
 * Position to "new" in the scrolling list.
 * Set the "new" data in the selected field.
 */

void
UnselectSelect()
{
	HostData *hp = sf->flatItems[sf->currentItem].pField;
#ifdef DEBUG
	fprintf(stderr,"UnselectSelect with sf->currentItem=%d\n",sf->currentItem);
#endif
	

	/* Select the new item */
	OlVaFlatSetValues ((Widget)
		sf->scrollingList,
		sf->currentItem,
		XtNset,	True,
		(String)0
	);
	XtVaSetValues ((Widget)
		sf->scrollingList,
		XtNviewItemIndex, sf->currentItem,
		(String)0
	);
	/* Reset the cusor to the first text field */
	AcceptFocus(sf->w_name);
	/* clear up the footer msg on the base window */
	CLEARMSG();

	ResetFields(hp);
} /* UnselectSelect */

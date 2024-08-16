/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umailsetup:actionArea.h	1.5"
#ifndef	ACTION_AREA_H
#define	ACTION_AREA_H

#include	<Xm/Xm.h>		//  ... always needed

#define	TIGHTNESS	20


//  Values to be used in the "which" element of the ActionAreaItem
#define	OK_BUTTON	1
#define	APPLY_BUTTON	2
#define	RESET_BUTTON	3
#define	CANCEL_BUTTON	4
#define	HELP_BUTTON	5



typedef struct _action_area_item
{
	char		*label;		//  label of the button
	char		*mnemonic;	//  mnemonic of the button
	int		which;		//  which button (OK, Apply, Help, etc)
	Boolean		sensitive;	//  sensitivity of action area button
	void   (*callback)(Widget, XtPointer clientData, XmAnyCallbackStruct *);
					//  pointer to callback routine
	void		*clientData;	//  client data for callback routine
} ActionAreaItem;




#endif	//  ACTION_AREA_H

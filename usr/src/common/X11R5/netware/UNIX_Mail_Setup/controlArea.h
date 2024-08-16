/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umailsetup:controlArea.h	1.2"
#ifndef	CONTROL_AREA_H
#define	CONTROL_AREA_H

#include	"setupAPIs.h"	//  for setupVar_t definition

typedef struct _buttonItem
{
	Widget	widget;		//  widget id of the button
	int	index;		//  index of this button in the button array
	char	*label;		//  label for the button
	char	*mnemonic;	//  mnemonic for the button
				//  the function to call when the button is pressed
	void	(*callback) (Widget w, XtPointer callData, XmAnyCallbackStruct *);
	setupVar_t *curVar;	//  the current setup variable we're working with
} ButtonItem;



#endif	CONTROL_AREA_H

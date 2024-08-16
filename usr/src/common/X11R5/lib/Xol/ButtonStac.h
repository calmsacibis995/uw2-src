/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)buttonstack:ButtonStac.h	1.11"
#endif

#ifndef _Ol_ButtonStac_h_
#define _Ol_ButtonStac_h_

/*************************************************************************
 *
 * Description:
 *		This is the "public" include file for the ButtonStack
 *	Widget and Gadget.
 *
 *****************************file*header********************************/

/***********************************************************************
 *
 * ButtonStack Widget
 *
 ***********************************************************************/

#include <Xol/MenuButton.h>

extern WidgetClass				buttonStackWidgetClass;
typedef struct _MenuButtonClassRec *		ButtonStackWidgetClass;
typedef struct _MenuButtonRec *			ButtonStackWidget;

extern WidgetClass				buttonStackGadgetClass;
typedef struct _MenuButtonGadgetClassRec *	ButtonStackGadgetClass;
typedef struct _MenuButtonGadgetRec *		ButtonStackGadget;

#endif /* _Ol_ButtonStac_h_ */

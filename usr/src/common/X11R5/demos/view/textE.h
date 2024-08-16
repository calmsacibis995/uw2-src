/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/textE.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: textE.h,v $ $Revision: 1.2.2.2 $ $Date: 1992/04/28 15:26:05 $ */

#if ( defined text_h )
#define extern 
#endif

#ifdef _NO_PROTO
extern void FileOKCallback();
extern void NewPaneCallback();
extern void KillPaneCallback();
extern void FindCallback();
#else
extern void FileOKCallback(Widget fsb, ViewPtr this,
			   XmFileSelectionBoxCallbackStruct *call_data);

extern void FindCallback(Widget button, ViewPtr this,
			  XmPushButtonCallbackStruct *call_data);

extern void NewPaneCallback(Widget fsb, ViewPtr this,
			    XmPushButtonCallbackStruct *call_data);

extern void KillPaneCallback(Widget button, ViewPtr this,
			    XmPushButtonCallbackStruct *call_data);

#endif

#if ( defined extern )
#undef extern 
#endif

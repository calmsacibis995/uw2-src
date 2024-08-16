/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_question.h	1.2"
/*----------------------------------------------------------------------------
 *
 * Filename: ps_question.h
 *
 * Description: This file defines the PSQuestion class. This is used
 *		to display information dialogs on the screen.
 *
 */
#ifndef PSQUESTION_H
#define PSQUESTION_H

/*----------------------------------------------------------------------------
 *
 */
class PSQuestion : public BasicComponent {
public:
								PSQuestion (Widget		parent,
											char*		msgStr,
											ClientInfo*	info);
								~PSQuestion ();

private:
	ClientInfo*					d_info;

private:
	//**********************************************************************
	// The Callbacks should be passed into the constructor.
	//**********************************************************************
    static void					OKCallback (Widget,
											XtPointer clientData,
											XtPointer);
    void						OK ();
    static void					CancelCallback (Widget,
												XtPointer clientData,
												XtPointer);
    void						Cancel ();
};

#endif   // PSQUESTION_H

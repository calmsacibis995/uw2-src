/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_msg.h	1.3"
/*----------------------------------------------------------------------------
 *	ps_msg.h
 */
#ifndef PSMSG_H
#define PSMSG_H

/*----------------------------------------------------------------------------
 *
 */
class PSMsg : public BasicComponent {
public:
							    PSMsg (Widget parent,
									   char* name,
									   char* msgStr);// "str:1" FS "text" format
    							~PSMsg ();

private:
    static void					OKCallback (Widget, XtPointer data, XtPointer);
};

/*----------------------------------------------------------------------------
 *
 */
class PSError {
public:
							    PSError (Widget	parent, char* message);
    							~PSError ();

private:
	Widget						d_w;

private:
    static void					OKCallback (Widget, XtPointer data, XtPointer);
};

#endif   // PSMSG_H

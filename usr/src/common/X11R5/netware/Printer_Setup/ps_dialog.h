/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_dialog.h	1.6"
/*----------------------------------------------------------------------------
 *	ps_dialog.h
 */
#ifndef PSDIALOG_H
#define PSDIALOG_H

/*----------------------------------------------------------------------------
 *
 */
class PSPrinter;

class PSDialog : public PSWin {
public:								// Constructors/Destructors
								PSDialog (Widget		parent,
										  char*			name,
										  PSPrinter*	printer,
										  short			ptype,
										  action*		abi,
										  short			buttonCnt);
								~PSDialog ();	

private:							// Private Data
	Widget						d_dialog; 		// DialogShell Widget
	Widget						d_panedWindow; 	// PanedWindow Widget
	Widget						d_actionArea;	// Form for Action Area Widget
	Widget						d_ctrlArea;		// Form for Control Area Widget

private:							// Private Methods
	static void					CloseCB (Widget,
										 XtPointer clientData,
										 XtPointer);
	void						CloseDialog ();
	Widget 						CreateActionArea (action* abi, short buttonCnt);

public:								// Public interface methods
	void   						RaiseDialogWin ();
	void   						ShowDialog (); 
	void						turnOffSashTraversal ();
	void						unmanage ();

public:								// Public inline interface methods
	inline Widget				GetDialog ();
	inline Widget				GetCtrlArea ();
	inline Widget				panedWindow ();
};

/*----------------------------------------------------------------------------
 *
 */
Widget
PSDialog::GetDialog ()
{
	return (d_dialog);
};

Widget
PSDialog::GetCtrlArea ()
{
	return (d_ctrlArea);
};

Widget
PSDialog::panedWindow ()
{
	return (d_panedWindow);
};

#endif		// PSDIALOG_H

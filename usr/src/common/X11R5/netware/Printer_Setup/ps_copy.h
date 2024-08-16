/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_copy.h	1.7"
/*----------------------------------------------------------------------------
 * ps_copy.h
 */
#ifndef PSCOPY_H
#define PSCOPY_H

#include "fileSelect.h"

/*----------------------------------------------------------------------------
 *
 */
class PSCopy : public PSDialog {
public: 
								PSCopy (Widget		parent,
										char*		name,
										PSPrinter*	selPrinter,
										short		ptype,
										action*		abi,
										short		buttonCnt);
								~PSCopy ();

private:
	fileSelect*					d_fb;
	PSPrinter*					d_selPrinter;

	Widget						d_pathLabel;
	uid_t						d_owner;
	gid_t						d_group;
	char*						d_homeDir;

private: 
	void						CreateCtrlArea ();

	static void					copyOKCallback (Widget,
												XtPointer data,
												XtPointer);
	static void					copyCancelCallback (Widget,
													XtPointer data,
													XtPointer);
	static void					copyHelpCallback (Widget,
												  XtPointer data,
												  XtPointer);
	void						copyOK ();
	void						copyCancel ();
	void						copyHelp ();

	static void					UpdatePathLblCallback (void* ptr);
	void						UpdatePathLbl ();
};

#endif		// PSCOPY_H

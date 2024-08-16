/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_win.h	1.5"
/*----------------------------------------------------------------------------
 *	ps_win.h
 *
 *	This file contains the definition for the PSWindow class. This class is
 *	the base class for all the window classes in the Printer_Setup program
 *	(except for the MainWin class).
 */
#ifndef PSWIN_H
#define PSWIN_H

#include <Xm/Xm.h>

const short						PSWIN_NO_TYPE 			= 0;
const short						PSWIN_ADD_LOCAL_TYPE	= 1;
const short						PSWIN_ADD_REMUNIX_TYPE	= 2;
const short						PSWIN_ADD_NETWARE_TYPE	= 3;
const short						PSWIN_UPDATE_TYPE 		= 4; 
const short						PSWIN_COPY_TYPE			= 5;
const short						PSWIN_CTRL_TYPE			= 6;
const short						PSWIN_REMACCESS_TYPE	= 7;
const short						PSWIN_USERACCESS_TYPE	= 8;
const short						PSWIN_NPRINTER_TYPE		= 9;
const short						PSWIN_MKDFLT_TYPE		= 10;
const short						PSWIN_HIDETOOLBAR_TYPE	= 11;
const short						PSWIN_SHOWTOOLBAR_TYPE	= 12;
const short						PSWIN_DELETE_TYPE		= 13;
const short						PSWIN_APPHELP_TYPE		= 14;
const short						PSWIN_TOCHELP_TYPE		= 15;
const short						PSWIN_DSKHELP_TYPE		= 16;
const short						PSWIN_EXIT_TYPE			= 17;

/*----------------------------------------------------------------------------
 *
 */
class PSWin : public BasicComponent {
public:
								PSWin (Widget parent, char* name, short ptype);
								~PSWin ();

private:
	char*						d_printerName;
	short						d_ptype;  

public:
	PSWin*						d_next;						// Should be Private
	PSWin*						d_prev;						// Should be Private
	Boolean						d_delete;					// Should be Private

public:
	Boolean						FindMatch (char* printerName, short ptype);
 	virtual void				RaiseDialogWin () = 0;	

public:
	inline char*				GetPrinterName ();
};

/*----------------------------------------------------------------------------
 *
 */
char*
PSWin::GetPrinterName ()
{
	return (d_printerName);
}

#endif

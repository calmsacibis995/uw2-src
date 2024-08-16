/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_nprinter.h	1.10"
/*----------------------------------------------------------------------------
 *	ps_nprinter.h
 */
#ifndef PSNPRINTER_H
#define PSNPRINTER_H

#undef Status
#include <libnpt.h>

//--------------------------------------------------------------
//           			T Y P E D E F S  
//--------------------------------------------------------------
typedef struct __nprinterstatus {
	char*						localName;
	char*						remoteName;
	uint8						slot;
	char*						pServer;
} NPrinterStatus;

/*----------------------------------------------------------------------------
 *
 */
class PSNPrinter : public PSDialog {
public:								// Constructors/Desctuctors
								PSNPrinter (Widget		parent,
											char*		name,
											PSPrinter*	selPrinter,
											short		ptype,
											action*		abi, 
											short		buttonCnt);
								~PSNPrinter ();

private:							// Private methods
	void						CreateCtrlArea ();

	static void					connectCallback (Widget,
												 XtPointer clientData,
												 XtPointer);
	void						connect ();
	static void					cancelCallback (Widget,
											    XtPointer clientData,
											    XtPointer);
	void						cancel ();
	static void					helpCallback (Widget,
											  XtPointer clientData,
											  XtPointer);
	void						help ();
	static void					doDisconnectCallback (Widget,
													  XtPointer clientData,
													  XtPointer);
	void						doDisconnect ();
	void						selectFileServer (XmListCallbackStruct* cbs);
	static void					selectFileServerCallback (Widget,
														  XtPointer clientData,
														  XtPointer callData);
	void						selectPrintServer (XmListCallbackStruct* cbs);
	static void					selectPrintServerCallback (Widget,
														   XtPointer clientData,
														   XtPointer callData);
	void						selectPrinter (XmListCallbackStruct* cbs);
	static void					selectPrinterCallback (Widget,
													   XtPointer clientData,
													   XtPointer callData);

private:							// Private data
	short						CHARS_PER_LIST;

	Widget						d_parent;
	Widget						_dialog;
	XmString*					_printServers;

	NPrinterStatus*				_nPrinterStatus;

	PSPrinter*					_selPrinter;

	char*						_selFileServer;
	char*						_selPrintServer;
	char*						_selPrinterName;
	uint8						_printerSlot;
	uint8						_printerSlots[2048];// THIS NEEDS TO BE DYNAMIC

	int							_printServerPixmap;

	PList*						d_fileServers;
	PList*						d_printServers;
	PList*						d_remotePrinters;
	char**						_nwFileServers;

private:
	NPrinterStatus*				GetNPrinterStatus ();	
	void						FreeNPrinterStatus (NPrinterStatus* status);
	void						AddNPrinterStatus (NPrinterStatus* status);
	void						DelNPrinterStatus (NPrinterStatus* status);
	NPrinterStatus*				BuildNPrinterStatus ();
	Boolean						CheckForDuplicate (NPrinterStatus* status);
	short						CountEntries ();
};

#endif

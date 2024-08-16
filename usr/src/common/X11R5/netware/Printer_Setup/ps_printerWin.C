#ident	"@(#)prtsetup2:ps_printerWin.C	1.16"
/*----------------------------------------------------------------------------
 *	ps_printerWin.c
 */

#include <iostream.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <Xm/Xm.h>

#include <Dt/Desktop.h>
#include <Dt/DtMsg.h>

#include "BasicComponent.h"
#include "ps_mainwin.h"

#define MY_QUEUE	XInternAtom (XtDisplay(w), "_DT_QUEUE", False)

#ifndef 	PATH_MAX
#define		PATH_MAX	1024
#endif

#define _DEFAULT_PRINTER		"_DEFAULT_PRINTER"

extern "C" void					InitializeResourceBuffer ();
extern "C" int					AppendToResourceBuffer (char* application,
														char* name,
														char* value);
extern "C" void					SendResourceBuffer (Display*
													dpy,
													Window	client,
													int		serial,
													char*	name);

extern const char*				DOT_PRINTER; 
extern DispInfo*				s_di;

//--------------------------------------------------------------
//	This is the constructor for the PSPrinterWin class
//--------------------------------------------------------------
PSPrinterWin::PSPrinterWin (Widget			parent,
							char*			name,
							char**			pixmapFiles,
							short			pixmapCnt,
							Application*	ps,
							void*			mainWin)
			: PRT_MGR_STR ("$XWINHOME/bin/PrtMgr -o -p %s &"),
			  IconWin (parent, name, pixmapFiles, pixmapCnt)
{
	_dfltPrinter = 0;
	_mainWin = mainWin;
	_dfltPrinter = ps->GetDfltPrinter ();
	XtAddEventHandler (ps->baseWidget (),
					   (EventMask)NoEventMask,
					   True,
					   &PSPrinterWin::dtmResponseCB,
					   (XtPointer)this);
	DtInitialize (ps->baseWidget ());
	errno = ENOENT;
	GetPrinters ();
	if (errno != ENOENT && errno != ENODATA) {
		cerr << "Error:  getprinters (" << errno << ")" << endl;
		new PSError (_w, GetLocalStr (TXT_gettingPrinters));
	}
	ReGenWin (True);
} 

//--------------------------------------------------------------
//	This is the destructor for the PSPrinterWin class
//--------------------------------------------------------------
PSPrinterWin::~PSPrinterWin ()
{
}

//--------------------------------------------------------------
//	Get the list of printers 
//--------------------------------------------------------------
void
PSPrinterWin::GetPrinters ()
{
	PRINTER*					printer;

	while (printer = getprinter ("all")) {
		InsertPrinter (printer);
	}
}

//--------------------------------------------------------------
//	Add a printer to the list in iconWin.
//--------------------------------------------------------------
void
PSPrinterWin::AddPrinter (char* name)
{
	PRINTER*					printer;

	printer = getprinter (name); 
	if (printer) {
		InsertPrinter (printer);
		ReGenWin ();
	}
}

//--------------------------------------------------------------
//	This function adds a printer to the internal list of printers.
//--------------------------------------------------------------
void
PSPrinterWin::InsertPrinter (PRINTER* printer)
{
	PSPrinter*					tmp;

	tmp = new PSPrinter (printer);
	_list->Insert (tmp);
	UpdatePixmap (tmp);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinterWin::AddNewPrinter (PSPrinter* newPrinter)
{
	_list->Insert (newPrinter);
	UpdatePixmap (newPrinter);
	AddIcon (newPrinter);
}

//--------------------------------------------------------------
//	This function adds a printer with a NULL name to 
//	the internal list of printers.
//--------------------------------------------------------------
PSPrinter*
PSPrinterWin::AddNULLPrinter ()
{
	PRINTER						printer;

	printer.name = NULL;

	PSPrinter*					newPrinter = new PSPrinter (&printer);
	return (newPrinter);
}

//--------------------------------------------------------------
//	This function deletes the currently selected printer.
//--------------------------------------------------------------
void
PSPrinterWin::DeletePrinter ()
{
	PSPrinter*					selectedPrinter;

	selectedPrinter = (PSPrinter*)_list->GetSelected ();
	DeleteIcon (selectedPrinter);
	_list->Remove (selectedPrinter);
}

//--------------------------------------------------------------
//	This function sets the selected printer to the default printer.
//--------------------------------------------------------------
void
PSPrinterWin::SetDfltPrinter ()
{
	DtRequest					request;
	PSPrinter*					printer;
	PSPrinter*					curDflt;

	printer = GetSelectedPrinter ();
	curDflt = GetDefaultPrinter ();
	_dfltPrinter = printer->GetLabel ();
	UpdatePixmap (printer);
	if (curDflt != NULL) {
		UpdatePixmap (curDflt);
	}

	ReGenWin ();					// Change this to ReDrawWin for efficiency 

	InitializeResourceBuffer ();
	AppendToResourceBuffer ("", "*defaultPrinter", printer->GetLabel ());
	SendResourceBuffer (XtDisplay (_w), s_di->GetWindow (), 0, "prtsetup");
	memset (&request, 0, sizeof (request));
	request.set_property.rqtype = DT_SET_DESKTOP_PROPERTY;
	request.set_property.name = _DEFAULT_PRINTER;
	request.set_property.value = printer->GetLabel ();
	request.set_property.attrs = 0;
	DtEnqueueRequest (XtScreen (_w),
					  _DT_QUEUE (XtDisplay (_w)),
					  _DT_QUEUE (XtDisplay (_w)),
					  s_di->GetWindow (), &request);
} 

//--------------------------------------------------------------
//	This function gets the default printer from the IconList *_list object.
//--------------------------------------------------------------
PSPrinter*
PSPrinterWin::GetDefaultPrinter ()
{
	PSPrinter*					printer;

	printer = (PSPrinter*)_list->FindObj (_dfltPrinter);
	return (printer);
}

//--------------------------------------------------------------
//	This function gets the selected printer from the IconList *_list object.
//--------------------------------------------------------------
PSPrinter*
PSPrinterWin::GetSelectedPrinter ()
{
	PSPrinter*					printer;

	printer = (PSPrinter*)_list->GetSelected ();
	return (printer);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinterWin::NotifyParentOfSel (IconObj* obj)
{
	 ((PSMainWin*)_mainWin)->UpdateSelectedPrinter (obj);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinterWin::NotifyParentOfDblSel (IconObj* obj)
{
	char*						tmp;

	((PSMainWin*)_mainWin)->UpdateSelectedPrinter (obj);

	tmp = new char[strlen (PRT_MGR_STR) + strlen (obj->GetLabel ()) + 1];
	sprintf (tmp, PRT_MGR_STR, obj->GetLabel ());
	noPrivSystem (tmp); 
	delete (tmp);
}

//--------------------------------------------------------------
//	This function updates the pixmap assoc. with a
//	printer. Currently used when a printer is inserted 
//	or when	a printer becomes the default printer or is 
//	no longer the default printer.
//--------------------------------------------------------------
void
PSPrinterWin::UpdatePixmap (PSPrinter* tmp)
{
	short						pixPos = 0;
	char*						pix;

	pixPos += (strcmp (_dfltPrinter, tmp->GetLabel ()) == 0) ? 1 : 0;
	switch (tmp->GetProtocol ()) {
	case NUC:
		pixPos += NETWARE_PTR;
		break;

	case BSD:
	case SysV:
		pixPos += UNIX_PTR;
		break;

	default:
		pixPos += LOCAL_PTR;
		break;
	}
	pix = GetPixmap (pixPos);
	tmp->UpdatePixmap (pix);
}

//--------------------------------------------------------------
//	This function is called when the response to a desktop API is received.
//--------------------------------------------------------------
void
PSPrinterWin::dtmResponseCB (Widget		w,
							 XtPointer	clientData,
							 XEvent*	xEvent,
							 Boolean*)
{
	PSPrinterWin*				obj = (PSPrinterWin*)clientData;

	obj->dtmResponse (w, xEvent); 
}

//--------------------------------------------------------------
//	This function is called from dtmResponseCB. It is
//	the member function to handle desktop APIs
//--------------------------------------------------------------
void
PSPrinterWin::dtmResponse (Widget w, XEvent* xEvent)
{
	DtReply 					reply;
	//ExmFlatCallData			itemData;
	PSPrinter*					obj;

	if ((xEvent->type != SelectionNotify) 	||
								 (xEvent->xselection.selection != MY_QUEUE)) {
			return; 
	}
	memset (&reply, 0, sizeof (reply));
	DtAcceptReply (XtScreen (w), MY_QUEUE, XtWindow (w), &reply);

	if (reply.header.status == 0) {
		Arg 					args[2];
		DtPropList 				plist;
		char*					val;

		memset (&plist, 0, sizeof (DtPropList));
		// Set %F to full path name 
		DtAddProperty (&plist, "F", reply.get_fclass.file_name, NULL);
		DtAddProperty (&plist, "f", reply.get_fclass.file_name, NULL);
		XtSetArg (args[0], XmNuserData,  (XtArgVal)&obj); 
		ExmFlatGetValues (_widg, DroppedIndex (), args, 1);
		DtAddProperty (&plist, "_DEFAULT_PRINTER", obj->GetLabel (), NULL);
		if (val = DtGetProperty (&(reply.get_fclass.plist), "_Print", NULL)) {
			noPrivSystem (DtExpandProperty (val, &plist));
		}
		else {
			new PSError (_w, GetLocalStr (TXT_cannotPrint));
		}
		//DtFreePropertyList (&plist);
	}
	//DtFreeReply (&reply);
}

//--------------------------------------------------------------
//	This is virtual function called from TransferProc in IconWin
//--------------------------------------------------------------
void
PSPrinterWin::Transfer (Widget w, DmDnDDstInfoPtr dip)
{
	DtRequest					req;

	_widg = w;

	if (dip->error || dip->nitems == 0) {
		return;
	}
	for (int i = 0; i < dip->nitems; i++) {
		memset (&req, 0, sizeof (req));
		req.header.rqtype = DT_GET_FILE_CLASS;
		req.get_fclass.file_name = dip->files[i];
		req.get_fclass.options = DT_GET_PROPERTIES;
		DtEnqueueRequest (XtScreen (w),
						  _DT_QUEUE (XtDisplay (w)),
						  _DT_QUEUE (XtDisplay (w)),
						  s_di->GetWindow (),
						  &req);
	}	
}

//--------------------------------------------------------------
//	This is virtual function called from DropProc in IconWin
//--------------------------------------------------------------
void
PSPrinterWin::DropProc (Widget w, XtPointer clientData, XtPointer callData)
{
	ExmFlatDropCallData*		d = (ExmFlatDropCallData*)callData;

	switch (d->reason) {	
	case ExmINTERNAL_DROP:
		break;

	case ExmEXTERNAL_DROP:
		{
			char**				list;
			char*				tmp;
			char*				home;
			char*				prtDir;
			struct stat			stats;
			IconObj*			selPrinter;
			
			selPrinter = (IconObj*)(d->item_data.item_user_data);

			tmp = getenv("HOME");
			if (!tmp) {
				home = strdup("/");
			}
			else {
				home = strdup (tmp);
			}
			// Create $HOME/.printer path
			prtDir = XtMalloc (strlen (home) + strlen (DOT_PRINTER) + 1 + 1); 
			sprintf (prtDir, "%s/%s", home, DOT_PRINTER);

			// Check for prtDir. Create if necc. and change
			// permissions to real owner since we may be running privileged.	
			if (stat (prtDir, &stats) != 0) {
				if (errno != ENOENT || 
					mkdir (prtDir,
						   S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
					PSMsg*		pm = new PSMsg (_w, "noPrtDir", TXT_noPrtDir);
					XtFree (prtDir);
					return;
				}
			}

			// Now create the $HOME/.printer/<printerName> string
			prtDir = XtRealloc (prtDir, strlen (prtDir) + 1 +
								strlen (selPrinter->GetLabel ()) + 1);
			sprintf (prtDir, "%s/%s", prtDir, selPrinter->GetLabel ());
#ifdef DEBUG
			printf ("The printDir is %s\n", prtDir);
#endif
			if (access (prtDir, R_OK)) { 
				int				fd;

				fd = creat (prtDir, S_IRWXU | S_IRWXG | S_IRWXO);
				if (fd < 0) {
					PSMsg*		pm = new PSMsg (_w, "noPrtDir", TXT_badInstall);
					XtFree (prtDir);
					return;
				}
				else {
					close (fd);
					chown (prtDir, getuid (), getgid ()); 
				}
			}		

			list = (char**)XtMalloc (sizeof (char*) * 2);
			list[0] = strdup (prtDir);
			list[1] = NULL;
			if (DmDnDNewTransaction (_w,
									 list,
									 0,
									 d->selection,
									 XmDROP_LINK,
									 NULL,
									 NULL,
									 NULL)) {
				cerr << "DropProc: failed to get sip" << endl;
			}
			if (home) {
				free (home);
			}
			if (prtDir) {
				XtFree (prtDir);
			}
		}	
		break;
	
	default:
		break;
	}
}


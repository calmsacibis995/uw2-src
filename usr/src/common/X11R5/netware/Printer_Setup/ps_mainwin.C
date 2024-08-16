#ident	"@(#)prtsetup2:ps_mainwin.C	1.9"
/*----------------------------------------------------------------------------
 *	ps_mainwin.c
 */
#include <iostream.h>

#include <Xm/Form.h>

#include "BasicComponent.h"
#include "ps_mainwin.h"
#include "ps_dialog.h"
#include "ps_i18n.h"

#define PSMAINWIN_REALLY_DELETE			1
#define PSMAINWIN_ACTIVE_JOBS			2

char*							pixmapFiles[PIXMAP_CNT] = { 
										{ "/usr/X/lib/pixmaps/ptr.stp32" },
										{ "/usr/X/lib/pixmaps/dfltprt.icon" },
										{ "/usr/X/lib/pixmaps/ptr.rmt32" },
										{ "/usr/X/lib/pixmaps/ptrrmt.def32" },
										{ "/usr/X/lib/pixmaps/ptr.nw32" },
										{ "/usr/X/lib/pixmaps/ptrnw.def32" } };
char*							PRINTER_WIN = { "PrinterWin" };
char*							ACTIONS = { "Actions" };

/*----------------------------------------------------------------------------
 *
 */
PSMainWin::PSMainWin (Widget		parent,				// Parent widget
					  char*			name,				// Widget Name
					  Application*	ps)
		 : BasicComponent (name)
{
#ifdef DEBUG
	cerr << "PSMainWin (" << this << ") Constructor" << endl;
#endif

	_selPrinter = 0;
    _winList = 0;										// Initially, no Dialogs
	_ps = ps;

    _w = XmCreateForm (parent, _name, 0, 0);
	_actions = new PSActions (_w, ACTIONS, this);	// Create menu/toolbar

	// Create the PSPrinterWin object
	_printerWin = new PSPrinterWin (_w,
									PRINTER_WIN,
									pixmapFiles,
									PIXMAP_CNT,
									ps,
									this);  	

	XtVaSetValues (_printerWin->baseWidget (),
				   XmNtopAttachment, XmATTACH_WIDGET,
				   XmNtopWidget, _actions->GetToolbarWidget (),
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNrightAttachment, XmATTACH_FORM,
				   0);

	XtAddCallback (_w,
				   XmNhelpCallback, 
				   (XtCallbackProc)&PSMainWin::helpCallback,
				   this);

    manage ();
}

/*----------------------------------------------------------------------------
 *
 */
PSMainWin::~PSMainWin ()
{
#ifdef DEBUG
	cerr << "PSMainWin (" << this << ") Destructor" << endl;
#endif

	if (_actions) {
		delete (_actions);
	}
	if (_printerWin) {
		delete (_printerWin);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSMainWin::helpCallback (Widget, XtPointer clientData, XtPointer)
{
	PSMainWin*					obj = (PSMainWin*)clientData;

	obj->help ();
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSMainWin::help ()
{
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_helpSect);
} 

//--------------------------------------------------------------
// Member function that adds a PSWin object to the
//		_winList list of PSWin objects.
//--------------------------------------------------------------
void
PSMainWin::AddDialogWin (PSWin* newWin)
{
    PSWin*						win = _winList;

	newWin->d_prev = 0;
	newWin->d_next = 0;
    if (!_winList) {
		_winList = newWin;
	}
    else {
		while (win->d_next) {
	    	win = win->d_next;
		}
		win->d_next = newWin;
		newWin->d_prev = win;
    }

#ifdef DEBUGLINKLIST
	win = _winList;
	cerr << "winList = " << _winList << endl;
	while (win) {
		cerr << "Printer Name = " << win->GetPrinterName () << endl;
		cerr << "  win = " << win << endl;
		cerr << "    prev = " << win->d_prev << endl;
		cerr << "    next = " << win->d_next << endl;
		win = win->d_next;
	}
#endif
}

/*----------------------------------------------------------------------------
 *
 */
void
PSMainWin::DeleteDialogs ()
{
	PSWin*						win = _winList;
	PSWin*						next;

	while (win) {
		if (win->d_delete) {
			if (!win->d_prev) {
				_winList = win->d_next;
#ifdef DEBUG
				cerr << "Delete -> " << win << endl;
#endif
				delete (win);
				if (win = _winList) {
					_winList->d_prev = 0;
				}
			}
			else {
				if (!win->d_next) {
					win->d_prev->d_next = 0;
#ifdef DEBUG
					cerr << "Delete -> " << win << endl;
#endif
					delete (win);
					return;
				}
				else {
					win->d_prev->d_next = win->d_next;
					win->d_next->d_prev = win->d_prev;	
					next = win->d_next;
#ifdef DEBUG
					cerr << "Delete -> " << win << endl;
#endif
					delete (win);
					win = next;
				}
			}
		}
		else {
			win = win->d_next;
		}
	}
}

//--------------------------------------------------------------
// This function removes all the dialogs for the specified printer.
//--------------------------------------------------------------
void
PSMainWin::RemovePrinterDialogs (char* pName)
{
	PSWin*						win = _winList;
	PSWin*						next;

	while (win) {
		if ((strcmp (win->GetPrinterName (), pName) == 0)) {
			if (!win->d_prev) {
				_winList = win->d_next;
				delete (win);
				if (win = _winList) {
					_winList->d_prev = 0;
				}
			}
			else {
				if (!win->d_next) {
					win->d_prev->d_next = 0;
					delete (win);
					return;
				}
				else {
					win->d_prev->d_next = win->d_next;
					win->d_next->d_prev = win->d_prev;	
					next = win->d_next;
					delete (win);
					win = next;
				}
			}
		}
		else {
			win = win->d_next;
		}
	}
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void
PSMainWin::UpdateSelectedPrinter (IconObj* obj)
{
	_selPrinter = (PSPrinter*)obj;
	_actions->UpdateActionSensitivity (_selPrinter);
}

//--------------------------------------------------------------
// This function is called to notify the PSMainWin
//				that a new default printer has been selected.
//--------------------------------------------------------------
void
PSMainWin::NotifyParentOfDefault ()
{
	_printerWin->SetDfltPrinter ();
}

//--------------------------------------------------------------
// This function deletes a printer. Then removes the
//				printer icon, cleans up any dialogs assoc. with
//				the printer, and takes the printer out of any lists.
//--------------------------------------------------------------
void
PSMainWin::DeletePrinter ()
{
	_deletePrinterInfo.ptr = this;
	_deletePrinterInfo.type = PSMAINWIN_REALLY_DELETE;
	_pq = new PSQuestion (_w, TXT_reallyDelete, &_deletePrinterInfo);
}

//--------------------------------------------------------------
// Member function to search a window in the 
//				_winList and raise it.
//
// Parameters:	short ptype - The type of dialog window
//
// Return:	True - Dialog already exists and has been raised
//			False - Dialog does not exist
//--------------------------------------------------------------
Boolean
PSMainWin::RaiseDialogWin (short ptype)
{
	PSWin*						win = _winList;
	char*						printerName = 0;
	
	if (_selPrinter && 
		(ptype != PSWIN_ADD_LOCAL_TYPE) &&
		(ptype != PSWIN_ADD_REMUNIX_TYPE) &&
		(ptype != PSWIN_ADD_NETWARE_TYPE)) {
			printerName = _selPrinter->GetLabel ();
	}
	while (win) {
		if (win->FindMatch (printerName, ptype)) {
			win->RaiseDialogWin ();
			return (True);
		}	
		win = win->d_next;
	}
	return (False);
}

//--------------------------------------------------------------
// This function requests the PSPrinterWin class to 
//				add a NULL PSPrinter.
//
// Return: 	NULL if unable to create a new printer
//			Pointer to new PSPrinter on success
//--------------------------------------------------------------
PSPrinter*
PSMainWin::AddNULLPrinter ()
{
	return (_printerWin->AddNULLPrinter ());
}

//--------------------------------------------------------------
// This function adds a newly created printer
//--------------------------------------------------------------
void
PSMainWin::AddPrinter (PSPrinter* newPrinter)
{
	_printerWin->AddNewPrinter (newPrinter);	
}

/*----------------------------------------------------------------------------
 *
 */
const IconObj*
PSMainWin::findByName (const char* name)
{
	return (_printerWin->findByName (name));
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void
PSMainWin::ResponseCallback (short type, Boolean response)
{
	delete (_pq);

	switch (type) {
	case PSMAINWIN_REALLY_DELETE:
		if (response) {
			PurgePrinter ();
		}
		break;

	case PSMAINWIN_ACTIVE_JOBS:
		if (response) {
			if (!LpCancelAll (_selPrinter->Name ())) {
				_pm = new PSMsg (_w, "DeletePrinterError", TXT_badCancel);
			}
			else {
				PurgePrinter ();
			}
		}
		break;

	default:
		break;
	}
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void
PSMainWin::PurgePrinter ()
{
	int							status;
	
	status = LpDelete (_selPrinter->Name ());
	if  (status == MOK) {
		CleanupPrinter ();
	}
	else {
		switch (status) {
		case MBUSY:
			_deletePrinterInfo.ptr = this;
			_deletePrinterInfo.type = PSMAINWIN_ACTIVE_JOBS;
			_pq = new PSQuestion (_w, TXT_activeJobs, &_deletePrinterInfo);
			break;

		case MTRANSMITERR:
		default:
			_pm = new PSMsg (_w, "DeletePrinterError", TXT_badDelete);
			break;
		}
	}
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void
PSMainWin::CleanupPrinter ()
{
	RemovePrinterDialogs (_selPrinter->Name ());
	_printerWin->DeletePrinter ();
	delete (_selPrinter);
	UpdateSelectedPrinter (NULL);
}


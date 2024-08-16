#ident	"@(#)prtsetup2:ps_copy.C	1.11"
/*----------------------------------------------------------------------------
 *	ps_copy.c
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <X11/Xos.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_printer.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_copy.h"
#include "ps_msg.h"
#include "mnem.h"

extern const char*				DOT_PRINTER; 

/*----------------------------------------------------------------------------
 *
 */
PSCopy::PSCopy (Widget		parent,
				char*		name,
				PSPrinter*	selPrinter,
				short		ptype,
				action*		abi,
				short		buttonCnt) 
	   : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	char*						tmp;

	d_selPrinter = selPrinter;
	d_homeDir = 0;

	if (tmp = getenv ("HOME")) {
		if (d_homeDir = new char [strlen (tmp) + 1]) {
			strcpy (d_homeDir, tmp);
		}
	}

	d_owner = getuid (); 
	d_group = getgid (); 

	XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCopy::copyOKCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCopy::copyCancelCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCopy::copyHelpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSCopy::copyHelpCallback,
				   this);

	turnOffSashTraversal ();

	CreateCtrlArea ();
	ShowDialog ();
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSCopy::~PSCopy ()
{
	if (d_homeDir) {
		delete (d_homeDir);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::CreateCtrlArea ()
{
	XmString					str;
	XmString					str2;
	Widget 						form;
	Widget 						copyLabel;
	Widget 						sep;
	Widget 						copyTxtLabel;
	Widget 						asLabel;
	Widget 						ctrlArea;

	ctrlArea = GetCtrlArea ();	
	str = XmStringCreateLocalized (GetLocalStr (TXT_copyStr));
	copyLabel = XtVaCreateManagedWidget ("copyLabel",
										 xmLabelWidgetClass, ctrlArea,
										 XmNtopAttachment, XmATTACH_FORM,
										 XmNleftAttachment, XmATTACH_FORM,
										 XmNleftOffset, 15,
										 XmNlabelString, str,
										 0);

	str2 = XmStringCreateLocalized (GetLocalStr (TXT_toStr));
	asLabel = XtVaCreateManagedWidget ("asLabel",
									   xmLabelWidgetClass, ctrlArea,
									   XmNtopAttachment, XmATTACH_WIDGET,
									   XmNtopWidget, copyLabel,
									   XmNleftAttachment, XmATTACH_FORM,
									   XmNleftOffset, 15,
									   XmNlabelString, str2,
									   0);

	sep = XtVaCreateManagedWidget ("sep", xmSeparatorWidgetClass,
								   ctrlArea,
								   XmNorientation, XmVERTICAL,
								   XmNseparatorType, XmNO_LINE,
								   XmNtopAttachment, XmATTACH_FORM,
								   XmNleftAttachment, XmATTACH_WIDGET,
								   XmNleftWidget, XmStringLength (str) > XmStringLength (str2) ? copyLabel : asLabel, 
								   0);	
	XmStringFree (str);
	XmStringFree (str2);

	str = XmStringCreateLocalized (d_selPrinter->Name ());
	copyTxtLabel = XtVaCreateManagedWidget ("copyTxtLbl", xmLabelWidgetClass,
											ctrlArea,
											XmNtopAttachment, XmATTACH_FORM,
											XmNleftAttachment, XmATTACH_WIDGET,
											XmNleftWidget, sep,	
											XmNalignment, XmALIGNMENT_BEGINNING,
											XmNlabelString, str,
											0);
	XmStringFree (str);

	str = XmStringCreateLocalized (d_homeDir);
	d_pathLabel = XtVaCreateManagedWidget ("pathLabel", xmLabelWidgetClass,
										   ctrlArea,
										   XmNtopAttachment, XmATTACH_WIDGET,
										   XmNtopWidget, copyTxtLabel,
										   XmNleftAttachment, XmATTACH_WIDGET,
										   XmNleftWidget, sep,
										   XmNalignment, XmALIGNMENT_BEGINNING,
										   XmNlabelString, str,
										   0);
	XmStringFree (str); 

	form = XtVaCreateWidget ("form",
							 xmFormWidgetClass, ctrlArea,
							 XmNtopAttachment, XmATTACH_WIDGET,
							 XmNtopWidget, asLabel,
							 XmNbottomAttachment, XmATTACH_FORM,
							 XmNleftAttachment, XmATTACH_FORM,
							 XmNrightAttachment, XmATTACH_FORM,
							 0);

	d_fb = new fileSelect (form, d_homeDir);
	XtManageChild (form);
	XtVaSetValues (d_fb->widget (), 
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNrightAttachment, XmATTACH_FORM,
				   0);
	d_fb->setUpdateCallback (UpdatePathLblCallback, this);
	d_fb->manage ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyOKCallback (Widget, XtPointer data, XtPointer)
{
	PSCopy*						obj = (PSCopy*)data;

	obj->copyOK ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyOK ()
{
	struct stat					stats;
	char*						prtDir;
	char*						realPrt;
	char*						tmp;
	char*						format;

	if (d_homeDir) {							// Create $HOME/.printer path
		prtDir = XtMalloc (strlen (d_homeDir) + strlen (DOT_PRINTER) + 2);
		sprintf (prtDir, "%s/%s", d_homeDir, DOT_PRINTER);
	}
	else {
		prtDir = XtMalloc (strlen (DOT_PRINTER) + 2);
		sprintf (prtDir, "/%s", DOT_PRINTER);
	}

	// Check for $HOME/.printer dir. Create if necc. Change to real
	// owner since we may be running privileged.
	if (stat (prtDir, &stats) != 0) {
		if (errno != ENOENT ||
			mkdir (prtDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			new PSError (_w, GetLocalStr (TXT_noPrtDir));
			XtFree (prtDir);
			return;
		}	
		chown (prtDir, d_owner, d_group);
	}
	else {
		if (!S_ISDIR (stats.st_mode)) {
			new PSError (_w, GetLocalStr (TXT_noPrtDir));
			XtFree (prtDir);
			return;
		}
	}

	// Now create the $HOME/.printer/<printerName> string
	prtDir = XtRealloc (prtDir,
						strlen (prtDir) + strlen (d_selPrinter->Name ()) + 2);
	sprintf (prtDir, "%s/%s", prtDir, d_selPrinter->Name ());
	if (access (prtDir, R_OK)) {
		int						fd;

		if ((fd = creat (prtDir, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
			new PSError (_w, GetLocalStr (TXT_badInstall)); 	
			XtFree (prtDir);
			return;
		}
		else {
			close (fd);
			chown (prtDir, d_owner, d_group);
		}
	}

	tmp = d_fb->getPathName ();			
	realPrt = new char[strlen (tmp) + strlen (d_selPrinter->Name ()) + 2];
	sprintf (realPrt, "%s/%s", tmp, d_selPrinter->Name ());
	if (symlink (prtDir, realPrt)) {
		switch (errno) {
		case EEXIST:
			format = GetLocalStr (TXT_exists);
			tmp = new char [strlen (realPrt) + strlen (format) + 1];
			sprintf (tmp, format, realPrt);
			new PSError (_w, tmp); 	
			delete (tmp);
			break;
		case ENOENT:
		case ENOTDIR:
			new PSError (_w, GetLocalStr (TXT_badPath)); 	
			break;
		default:
			new PSError (_w, GetLocalStr (TXT_badInstall)); 	
			break;
		}
		XtFree (prtDir);
		delete (realPrt);
		return;
	}

	XtFree (prtDir);
	delete (realPrt);
	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyCancelCallback (Widget, XtPointer data, XtPointer)
{
	PSCopy*						obj = (PSCopy*)data;

	obj->copyCancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyCancel ()
{
	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyHelpCallback (Widget, XtPointer data, XtPointer)
{
	PSCopy*						obj = (PSCopy*)data;

	obj->copyHelp ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::copyHelp ()
{
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_copySect);
}

/*----------------------------------------------------------------------------
 *	File selection callback
 */
void
PSCopy::UpdatePathLblCallback (void* ptr)
{
	PSCopy*						obj = (PSCopy*)ptr;

	obj->UpdatePathLbl ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCopy::UpdatePathLbl ()
{
	XmString					str;

	str = XmStringCreateLocalized (d_fb->getPathName ());
	XtVaSetValues (d_pathLabel, XmNlabelString, str, 0);
	XmStringFree (str);
}


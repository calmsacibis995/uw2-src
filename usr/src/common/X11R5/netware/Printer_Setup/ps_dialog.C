#ident	"@(#)prtsetup2:ps_dialog.C	1.12"
/*----------------------------------------------------------------------------
 *	ps_dialog.c
 */

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/Form.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <mnem.h>

#include "ps_hdr.h"
#include "ps_mainwin.h"
#include "ps_dialog.h"
#include "menuItem.h"

#define	TIGHTNESS				200

/*----------------------------------------------------------------------------
 *
 */
PSDialog::PSDialog (Widget		parent,
					char*		name,
					PSPrinter*	printer,
					short		ptype,
					action*		abi,
					short		buttonCnt) 
		: PSWin (parent, printer->GetLabel (), ptype)
{
	Atom						atom;
	Arg 						args[16];
	Cardinal					argCnt;

	argCnt = 0;
	XtSetArg (args[argCnt], XmNdeleteResponse, XmDESTROY);
	argCnt++;
	d_dialog = XmCreateDialogShell (XtParent (parent), name, args, argCnt);	
	_w = d_dialog;

	atom = XmInternAtom (XtDisplay (parent), "WM_DELETE_WINDOW", False);
	if (atom != None) {
   		XtVaSetValues (d_dialog, XmNdeleteResponse, XmDO_NOTHING, 0);
		XmAddWMProtocolCallback (d_dialog, atom, CloseCB, this);
	}
	d_panedWindow = XtVaCreateWidget ("pane", xmPanedWindowWidgetClass,
									  d_dialog, 
									  XmNsashWidth, 1,
									  XmNsashHeight, 1,
									  0); 

	d_ctrlArea = XtVaCreateWidget ("ControlArea",
								   xmFormWidgetClass,
								   d_panedWindow,
								   0); 

	d_actionArea = CreateActionArea (abi, buttonCnt);

	for (int i = 0; i < buttonCnt; i++) {
		//  Set the default button (so Return or Enter activates OK button)
		if (abi[i].which == BUTTON_OK) {
			XtVaSetValues (d_ctrlArea, XmNdefaultButton, abi[i].w, 0);
		}

		//  Set the cancel button (so ESC key activates Cancel button)
		if (abi[i].which == BUTTON_CANCEL) {
			XtVaSetValues (d_ctrlArea, XmNcancelButton, abi[i].w, 0);
		}
	}

	XtRealizeWidget (d_dialog);
} 

/*----------------------------------------------------------------------------
 *
 */
PSDialog::~PSDialog ()
{
}

/*----------------------------------------------------------------------------
 *	Raise the dialog 
 */
void
PSDialog::RaiseDialogWin ()
{
	if (d_dialog) {
		XRaiseWindow (XtDisplay (d_dialog), XtWindow (d_dialog));
	}
}

/*----------------------------------------------------------------------------
 *	Manage and pop up the dialog 
 */
void
PSDialog::ShowDialog ()
{
	XtManageChild (d_actionArea);
	XtManageChild (d_ctrlArea);
	XtManageChild (d_panedWindow);
	XtManageChild (d_dialog);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSDialog::turnOffSashTraversal ()
{
	Widget*						children;
	int							numChildren;

	XtVaGetValues (d_panedWindow,
				   XmNchildren,
				   &children,
				   XmNnumChildren,
				   &numChildren,
				   0);
	while (numChildren-- > 0) {
		if (XmIsSash (children[numChildren])) {
			XtVaSetValues (children[numChildren], XmNtraversalOn, False, 0);
		}
	}
}

/*----------------------------------------------------------------------------
 *	Create the Control Area for a Dialog 
 */
Widget
PSDialog::CreateActionArea (action* abi, short buttonCnt)
{
	Widget						actionArea;
	MenuItemClass*				item;
	Arg 						args[12];
	int 						argCnt;

	actionArea = XtVaCreateWidget ("ActionArea",
								   xmFormWidgetClass,
								   d_panedWindow, 
								   XmNfractionBase,
								   TIGHTNESS * buttonCnt - 1,
								   XmNskipAdjust,
								   TRUE,
								   0);
	for (int i = 0; i < buttonCnt; i++, abi++) {
		argCnt = 0;
		XtSetArg (args[argCnt],
				  XmNleftAttachment,
				  i ? XmATTACH_POSITION : XmATTACH_FORM); 			
		argCnt++;
		XtSetArg (args[argCnt], XmNleftPosition, TIGHTNESS * i); 	
		argCnt++;
		XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_FORM); 	
		argCnt++;
		XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM);
		argCnt++;
		XtSetArg (args[argCnt],
				  XmNrightAttachment, 
				  i != buttonCnt ? XmATTACH_POSITION : XmATTACH_FORM);
		argCnt++;
		XtSetArg (args[argCnt],
				  XmNrightPosition,
				  TIGHTNESS * i + (TIGHTNESS - 1));					
		argCnt++;
		XtSetArg (args[argCnt], XmNnavigationType, XmNONE);
		argCnt++;
		
		item = new MenuItemClass (actionArea,
								  abi->label,
								  abi,
								  args,
								  argCnt);
		abi->w = item->baseWidget ();

		if (i == 0) {
			// Set the pane window constraint for max and min heights
			// so this pane in the widget is not resizable
			Dimension			height;
			Dimension			h;
	
			XtVaGetValues (actionArea, XmNmarginHeight, &h, 0);
			XtVaGetValues (abi->w, XmNheight, &height, 0);
			height += 2 * h;
			XtVaSetValues (actionArea, 
						   XmNpaneMaximum, height,
						   XmNpaneMinimum, height,
						   0);
		}
		registerMnemInfo (abi->w,
						  GetLocalStr (abi->mnemonic),
						  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	}
	return (actionArea);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSDialog::CloseCB (Widget, XtPointer clientData, XtPointer)
{
	PSDialog*					obj = (PSDialog*)clientData;

	obj->CloseDialog ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSDialog::CloseDialog ()
{
	d_delete = True;
	XtUnmanageChild (_w);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSDialog::unmanage ()
{
	XtUnmanageChild (_w);
}


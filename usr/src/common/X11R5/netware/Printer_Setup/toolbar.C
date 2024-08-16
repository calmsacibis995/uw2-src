#ident	"@(#)prtsetup2:toolbar.C	1.6"
/*----------------------------------------------------------------------------
 *	toolbar.c
 */

#include <iostream.h>

#include <Xm/RowColumn.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>

#include "toolbar.h"
#include "ps_xpmbutton.h"

/*----------------------------------------------------------------------------
 *
 */
Toolbar::Toolbar (Widget		parent,
				  char*			name,
				  MenuBarItem	buttons[],
				  const short	cnt)
	   : SIDE_BORDER (6),
		 GROUP_PAD (32),
		 BasicComponent (name)
{
	Action*						cur;
   	XPMButton*					temp; 
   	Arg							args[16];
	Boolean						first = True;
	Boolean						found;
	short						grpCnt;
   	int		  					maxWidth = 0;
   	int							argCnt;
	int							pos1;
	int							pos2;

	_buttonList = NULL;

   	_w = XtVaCreateManagedWidget (name, xmFrameWidgetClass, parent, NULL);

   	_buttonForm = XtVaCreateWidget ("ButtonForm", xmFormWidgetClass, _w, 0);

	argCnt = 0;
	XtSetArg (args[argCnt], XmNeditable, False); argCnt++;
	XtSetArg (args[argCnt], XmNscrollHorizontal, False); argCnt++;
	XtSetArg (args[argCnt], XmNcursorPositionVisible, False); argCnt++;
	XtSetArg (args[argCnt], XmNwordWrap, True); argCnt++;
	XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNrightAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNtraversalOn, False); argCnt++;
	_promptLbl = XmCreateScrolledText (_buttonForm, "PromptText", args, argCnt);

   	// Determine the number of buttons needed   
   	found = True;
   	pos1 = SIDE_BORDER;
   	pos2 = SIDE_BORDER;
   	for (grpCnt = 1, _buttonCnt = 0; found == True; grpCnt++) {
		found = False;
		if (grpCnt != 1) {
			pos1 = pos2 += GROUP_PAD;
			maxWidth += GROUP_PAD;
		}
		for (int i = 0; i < cnt; i++) {
			action*				items;

			items = (&buttons[i])->action_items;
			if (items != NULL) {
				for (int j = 0; items[j].label; j++) {
					if (items[j].pix && items[j].group == grpCnt &&
																items[j].show) {
						found = True;
						argCnt = 0;
						XtSetArg (args[argCnt],XmNtopAttachment, XmATTACH_FORM);
						argCnt++;
						XtSetArg (args[argCnt],
								  XmNbottomAttachment,
								  XmATTACH_WIDGET);
						argCnt++;
						XtSetArg (args[argCnt], XmNbottomWidget, _promptLbl);
						argCnt++;
						temp = new XPMButton (_buttonForm,
											  &items[j],
											  _promptLbl,
											  args,
											  argCnt);

						pos2 += temp->GetWidth () + 6; 
						temp->SetPositions (pos1);
						maxWidth += temp->GetWidth () + 6;
						if (first) {
							_buttonList = temp;
							cur = temp;
							first = False;
						}
						else {
							cur->_next = temp;
							cur = temp;
						}
						_buttonCnt++;
						pos1 = pos2;
					}
				}
			}
		}
	}
	maxWidth -= GROUP_PAD;
	maxWidth += SIDE_BORDER + 6;

	for (cur = _buttonList; cur != NULL; cur = cur->_next) {
		cur->manage ();
	}

	XtAddEventHandler (_buttonForm,
					   StructureNotifyMask,
					   False,
					   &ResizeButtonFormCB,
					   (XtPointer)this);	

   	XtManageChild (_promptLbl);
   	XtManageChild (_buttonForm);
	manage ();
} 

/*----------------------------------------------------------------------------
 *	Callback called when the button form is resized
 */
void
Toolbar::ResizeButtonFormCB (Widget,
							 XtPointer	client_data,
							 XEvent*	event,
							 Boolean*)
{
	Toolbar*					obj = (Toolbar*)client_data;

	obj->ResizeButtonForm (event);
}

/*----------------------------------------------------------------------------
 *	Member function called from the ResizeButtonFormCB callback. This function
 *	sizes and positions the XPMButtons on the toolbar.
 */
void
Toolbar::ResizeButtonForm (XEvent* event)
{
	Action*						cur;
	int							pos1;
	int							pos2;

	if (event->type == ConfigureNotify) {
		pos1 = SIDE_BORDER;
		pos2 = SIDE_BORDER;
		cur = _buttonList; 
		for (int i = 0; i < _buttonCnt; i++, cur = cur->_next) {
			pos2 += ((XPMButton*)cur)->GetWidth () + 6;
			((XPMButton*)cur)->SetPositions (pos1);
			if (cur->_next) {
				if (cur->Group () < cur->_next->Group ()) {
					pos2 += GROUP_PAD;
				}
			}
			pos1 = pos2;
		}
	}
} 

/*----------------------------------------------------------------------------
 * This function sets the sensitivity for all the actions assoc. with the
 *	Toolbar. 
 *
 *	dfltFlag - If true set the sensitivity to the defualt value. If false set
 *	the sensitivity to the ~default value.
 */
void
Toolbar::SetSensitivity (Boolean dfltFlg)
{
	Action*						cur;

	for (cur = _buttonList; cur; cur = cur->_next) {
		cur->SetSensitivity (dfltFlg);
	}
}

/*----------------------------------------------------------------------------
 *	This function handles any special cases where sensitivity needs to be
 *	set. For example, NetWare vs UNIX, local vs remote, etc. 
 */
void
Toolbar::SpecialSensitivity (short special, Boolean newSensitivity)
{
	Action*						cur;

	for (cur = _buttonList; cur; cur = cur->_next) {
		cur->SpecialSensitivity (special, newSensitivity);
	}
}

/*----------------------------------------------------------------------------
 *	Member function to ess. hide the toolbar
 */
void
Toolbar::UnmanageButtonForm ()
{
	XtUnmanageChild (_buttonForm);
}

/*----------------------------------------------------------------------------
 *	Member function to show the toolbar
 */
void
Toolbar::ManageButtonForm ()
{
	XtManageChild (_buttonForm);
}


#ident	"@(#)prtsetup2:menuItem.C	1.3"
/*----------------------------------------------------------------------------
 *	menuItem.c
 */

#include <Xm/PushB.h>
#include <Xm/PushBG.h>

#include "menuItem.h"
#include "ps_hdr.h"

/*----------------------------------------------------------------------------
 *
 */
MenuItemClass::MenuItemClass (Widget	parent,
							  char*		name,
							  action*	menuItem,
							  Arg*		arg,
							  int		argCnt)
			 : Action (menuItem)
{
	char*						title;
	char*						mnemonic;
	XmString 					str;

	title = GetLocalStr (menuItem->label);

	_w = XmCreatePushButton (parent, title, arg, argCnt);
	manage ();

	if (menuItem->mnemonic) {
		mnemonic = GetLocalStr (menuItem->mnemonic);
		XtVaSetValues (_w, XmNmnemonic, *mnemonic, 0);
	}

	if (menuItem->accelerator && menuItem->accel_text) {
		str = XmStringCreateSimple (menuItem->accel_text);
		XtVaSetValues (_w,
					   XmNaccelerator,
					   menuItem->accelerator,
					   XmNacceleratorText,
					   str,
					   0);
		XmStringFree (str);
	}
	InitAction (menuItem);
} 

/*----------------------------------------------------------------------------
 *
 */
MenuItemClass::~MenuItemClass ()
{
	XtDestroyWidget (_w);
}


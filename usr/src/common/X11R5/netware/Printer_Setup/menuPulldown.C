#ident	"@(#)prtsetup2:menuPulldown.C	1.3"
/*----------------------------------------------------------------------------
 *	menuPulldown.c
 */

#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>

#include "menuPulldown.h"
#include "ps_hdr.h"

/*----------------------------------------------------------------------------
 *
 */
MenuPulldown::MenuPulldown (Widget		parent,
							char*		name,
							action*		menuP)
			: Action (menuP)
{
	XmString					str;
	char*						menuTitle;
	char*						menuMnemonic;

	menuTitle = GetLocalStr (menuP->label);
	menuMnemonic = GetLocalStr (menuP->mnemonic);

	_pulldown = XmCreatePulldownMenu (parent, name, NULL, 0);
	str = XmStringCreateSimple (menuTitle);
	_w = XtVaCreateManagedWidget (menuTitle,
								  xmCascadeButtonWidgetClass, parent,
								  XmNsubMenuId, _pulldown,
								  XmNlabelString, str,
								  XmNmnemonic, *menuMnemonic,
								  0);
	XmStringFree (str);

	InitAction (menuP);
} 

/*----------------------------------------------------------------------------
 *
 */
MenuPulldown::~MenuPulldown ()
{
}


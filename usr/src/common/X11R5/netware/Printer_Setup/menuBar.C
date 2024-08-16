#ident	"@(#)prtsetup2:menuBar.C	1.4"
/*----------------------------------------------------------------------------
 *	menuBar.c
 */

#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>

#include "menuBar.h"
#include "menuPulldown.h"
#include "menuItem.h"

/*----------------------------------------------------------------------------
 *
 */
MenuBar::MenuBar (Widget		parent,
				  char*			name,
				  MenuBarItem	menubar[],
				  const short	cnt)
	   : BasicComponent (name)
{
	MenuPulldown*				tmp;
	Action*						lastMenuBar = 0;

	_menuBarList = 0;
	_menuItemList = 0;

	_w = XmCreateMenuBar (parent, name, NULL, 0);
	for (int i = 0; i < cnt; i++) {
		tmp = BuildPulldownMenu (&menubar[i]);

		if (i == 0) {
			_menuBarList = tmp;
		}
		else {
			lastMenuBar->_next = tmp;
		}
		lastMenuBar = tmp;
		
		if (i == (cnt - 1)) {
			XtVaSetValues (_w, XmNmenuHelpWidget, tmp->baseWidget (), 0);
		}
	}
	manage ();
}

/*----------------------------------------------------------------------------
 *
 */
MenuBar::~MenuBar ()
{
	Action*						tmp;
	Action*						cur;

	XtDestroyWidget (_w);

	cur = _menuBarList;
	while (cur) {
		tmp = cur->_next;
		delete (cur);
		cur = tmp;	
	}

	cur = _menuItemList;
	while (cur) {
		tmp = cur->_next;
		delete (cur);
		cur = tmp;
	}
}

//--------------------------------------------------------------
//	This function builds a pulldown menu.
//--------------------------------------------------------------
MenuPulldown*
MenuBar::BuildPulldownMenu (MenuBarItem* menuBarItem)
{
	static Boolean				first = True;
	MenuPulldown*				pulldown;
	MenuItemClass*				menuItem;
	Action*						lastMenuItem;
	action*						items;

	items = menuBarItem->action_items;

 	pulldown = new MenuPulldown (_w, "Pulldown", &menuBarItem->menu_bar);

	if (items) {
		for (int i = 0; items[i].label; i++) {
			if (items[i].show) {
				menuItem = new MenuItemClass (pulldown->PulldownWidg (),
											  items[i].label,
											  &items[i],
											  NULL,
											  0);

				if (first) {
					_menuItemList = menuItem; 
					first = False;
				}
				else {
					lastMenuItem->_next = menuItem;
				}
				lastMenuItem = menuItem;
			}
		}
	}
	return (pulldown);
}

/*----------------------------------------------------------------------------
 *	Sets the sensitivity for all the actions assoc. with the
 *	menuBar. This includes both the menuPulldowns and the menuItems.
 *
 *	Boolean dfltFlg - If true set the sensitivity to the default value. If
 *	false set the sensitivity to the ~default value.
 */
void
MenuBar::SetSensitivity (Boolean dfltFlg)
{
	Action*						cur;

	for (cur = _menuBarList; cur; cur = cur->_next) {
		cur->SetSensitivity (dfltFlg);
	}
	for (cur = _menuItemList; cur; cur = cur->_next) {
		cur->SetSensitivity (dfltFlg);
	}
}

/*----------------------------------------------------------------------------
 * Sets the sensitivity for all the actions assoc.  with the menuBar. This
 *	includes both the the menuPulldowns and the menuItems.
 */
void
MenuBar::SpecialSensitivity (short special, Boolean newSensitivity)
{
	Action*						cur;

	for (cur = _menuBarList; cur; cur = cur->_next) {
		cur->SpecialSensitivity (special, newSensitivity);
	}
	for (cur = _menuItemList; cur; cur = cur->_next) {
		cur->SpecialSensitivity (special, newSensitivity);
	}
}


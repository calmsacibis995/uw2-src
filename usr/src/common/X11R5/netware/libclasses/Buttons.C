#ident	"@(#)libclass:Buttons.C	1.2"

////////////////////////////////////////////////////////////////////
// ButtonItem.C: A ButtonItem panel for all buttons 
/////////////////////////////////////////////////////////////////////
#include "Buttons.h"
#include "i18n.h"
#include <iostream.h>

/* SetLabels
 *
 * Set button item labels and mnemonics.
 */
void Buttons::SetLabels (ButtonItem *items, int cnt)
{
    char	*mnem;

    for ( ;--cnt>=0; items++)
    {
	items->_label =  (XtArgVal) I18n::GetStr ((char *) items->_label);
	mnem = I18n::GetStr ((char *) items->_mnem);
	items->_mnem = (XtArgVal) mnem [0];
    }
}	/* End of SetLabels */

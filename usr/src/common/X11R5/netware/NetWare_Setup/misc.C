#ident	"@(#)nwsetup:misc.C	1.2"
#ident	"@(#)nwsetup:misc.c	1.1"
#ident  "$Header: $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL Inc.                     			*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"misc.c	1.0"
#endif
*/

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include "nwsetup.h"

extern char  	*gettxt ();
/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    char	*sep;
    char	*str;

    sep = strchr (idstr, FS_CHR);
    *sep = 0;
    str = gettxt (idstr, sep + 1);
    *sep = FS_CHR;

    return (str);
}	/* End of GetStr () */

/* SetLabels
 *
 * Set menu item labels and mnemonics.
 */
void
SetLabels (MenuItem *items, int cnt)
{
    char	*mnem;

    for ( ;--cnt>=0; items++)
    {
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
	mnem = GetStr ((char *) items->mnem);
	items->mnem = (XtArgVal) mnem [0];
    }
}	/* End of SetLabels */

/*
 *		SetHelpLabels
 *
 * Set strings for help text.
 */
void
SetHelpLabels (HelpText *help)
{
    help->title = GetStr (help->title);
    if (help->section)
	help->section = GetStr (help->section);
}	/* End of SetHelpLabels */

 /*****************************************************************
 * Set button item labels.
 *****************************************************************/
void
SetButtonLbls (ButtonItem *items, int cnt)
{
	/* label the remaining items */
    	for ( ; --cnt>=0; items++)
		items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
}	/* End of SetButtonLbls */


#ident	"@(#)prtsetup2:ps_win.C	1.5"
/*----------------------------------------------------------------------------
 *	ps_win.c
 */

#include <iostream.h>

#include "BasicComponent.h"
#include "ps_win.h"

/*----------------------------------------------------------------------------
 *
 */
PSWin::PSWin (Widget parent, char* name, short ptype) 
	 : BasicComponent ("NotNeeded")
{
#ifdef DEBUG
	cerr << "PSWin (" << this << ") Constructor" << endl;
#endif

    d_next = 0;
    d_prev = 0;
	d_delete = False;

	d_ptype = ptype;
    _w = parent;				// Set to parent - overide in inherited classes 

    if (d_printerName = new char[strlen (name) + 1]) {
		strcpy (d_printerName, name);
	}
}

/*----------------------------------------------------------------------------
 *
 */
PSWin::~PSWin ()
{
#ifdef DEBUG
	cerr << "PSWin (" << this << ") Destructor" << endl;
#endif

    if (d_printerName) {
		delete (d_printerName);
	}
}

/*----------------------------------------------------------------------------
 *
 */
Boolean
PSWin::FindMatch (char* name, short type)
{
	if (!d_printerName) {
		return (FALSE);
	}
	return ((strcmp (d_printerName, name) == 0) && (d_ptype == type));
}

/*----------------------------------------------------------------------------
 *	Raise a window that is already on the screen.
 */
void
PSWin::RaiseDialogWin ()
{
#ifdef DEBUG
	cerr << "Pure Virtual method:  this should never happen" << endl;
#endif
}


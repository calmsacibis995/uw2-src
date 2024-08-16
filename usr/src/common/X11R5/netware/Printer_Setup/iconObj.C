#ident	"@(#)prtsetup2:iconObj.C	1.4"
/*----------------------------------------------------------------------------
 *	iconObj.c
 */

#include <string.h>

#include "iconObj.h"

/*----------------------------------------------------------------------------
 *
 */
IconObj::IconObj (char* label, char* pix)
{
#ifdef DEBUG
	cerr << "IconObj (" << this << ") Constructor" << endl;
#endif

	if (d_label = new char [strlen (label) + 1]) {
		strcpy (d_label, label);
	}
	d_pix = pix;
} 

/*----------------------------------------------------------------------------
 *
 */
IconObj::~IconObj ()
{
#ifdef DEBUG
	cerr << "IconObj (" << this << ") Destructor" << endl;
#endif

	if (d_label) {
		delete (d_label);
	}
} 

//--------------------------------------------------------------
//	Compares two IconObjs and returns True if they are equal, and False if
//	they are not equal.
//--------------------------------------------------------------
Boolean
IconObj::operator== (IconObj* obj)
{
	return (strcoll (this->d_label, obj->d_label) == 0);
}

//--------------------------------------------------------------
//	Compares two IconObjs and returns True if they are not equal, and False if
//	they are equal.
//--------------------------------------------------------------
Boolean
IconObj::operator!= (IconObj* obj)
{
	return (strcoll (this->d_label, obj->d_label) != 0);
}

//--------------------------------------------------------------
//	Compares two IconObjs and returns True if the first IconObj is greater
//	than the second IconObj.
//--------------------------------------------------------------
Boolean
IconObj::operator> (IconObj *obj)
{
	return (strcoll (this->d_label, obj->d_label) > 0);
}

/*----------------------------------------------------------------------------
 *
 */
void
IconObj::Label (char* label)
{
	if (d_label) {
		delete (d_label);
	}
	if (d_label = new char [strlen (label) + 1]) {
		strcpy (d_label, label);
	}
}


#ident	"@(#)prtsetup2:iconList.C	1.7"
/*----------------------------------------------------------------------------
 *	iconList.c
 */

#include <iostream.h>

#include <Xm/Xm.h>

#include "iconList.h"

//--------------------------------------------------------------
// This is the constructor for the IconObj class. 
//--------------------------------------------------------------
IconList::IconList ()
{
#ifdef DEBUG1
	cerr << "IconList (" << this << ") Constructor" << endl;
#endif

	d_list = 0;
	d_cur = 0;
	d_cnt = 0;
	d_selected = 0;
} 

//--------------------------------------------------------------
// This is the destructor for the IconList class
//--------------------------------------------------------------
IconList::~IconList ()
{
#ifdef DEBUG1
	cerr << "IconList (" << this << ") Destructor" << endl;
#endif

	IconObj*					ptr = d_list;
	IconObj*					tmp;

	while (ptr) {
		tmp = ptr->d_next; 
		delete (ptr);					// SHOULD THIS BE DONE HERE??????????
		ptr = tmp;
	}
} 

//--------------------------------------------------------------
//	This function inserts an iconObj into the list alphabetically. A future
//	enhancement would be to insert the object by class or based on some 
//	other criteria.
//--------------------------------------------------------------
void
IconList::Insert (IconObj* insObj)
{
	IconObj*					ptr = d_list;

	insObj->d_next = 0;
	insObj->d_prev = 0;

	if (!d_list) {
		d_list = insObj;
	}
	else {
		while (1) {
			if (strcmp (ptr->d_label, insObj->d_label) > 0) {
				if (!ptr->d_prev) {
					d_list = insObj;
				}
				else {
					insObj->d_prev = ptr->d_prev;
					ptr->d_prev->d_next = insObj;
				}
				insObj->d_next = ptr;
				ptr->d_prev = insObj;
				break;
			}
			else {
				if (!ptr->d_next) {
					ptr->d_next = insObj;
					insObj->d_prev = ptr;
					break;
				}
				else {
					ptr = ptr->d_next;
				}
			}
		}
	}
	d_cnt++;
}

//--------------------------------------------------------------
//	This function removes an IconObj from the IconList.
//--------------------------------------------------------------
void
IconList::Remove (IconObj* insObj)
{
	IconObj*					ptr = d_list;

	while ((ptr != insObj) && ptr) {
		ptr = ptr->d_next;
	}
	if (ptr) {
		if (!ptr->d_prev) {
			if (d_list = ptr->d_next) {
				d_list->d_prev = 0;
			}
		}
		else {
			ptr->d_prev->d_next = ptr->d_next;
			if (ptr->d_next) {
				ptr->d_next->d_prev = ptr->d_prev;	
			}
		}
		delete (ptr);					// SHOULD THIS BE DONE HERE??????????
		d_cnt--;
	}
}

//--------------------------------------------------------------
//	This function finds an IconObj in the list that
//	has the same label as the string passed in.
//--------------------------------------------------------------
IconObj*
IconList::FindObj (char* label)
{
	IconObj*					ptr = d_list;

	for (ptr = d_list; ptr != NULL; ptr = ptr->d_next) {
		if  (strcoll (ptr->GetLabel (), label) == 0) {
			return (ptr);
		}
	}
	return (0);
}

/*----------------------------------------------------------------------------
 *
 */
IconObj*
IconList::GetFirst ()
{
	d_cur = d_list;
	return (d_cur);
}

/*----------------------------------------------------------------------------
 *
 */
IconObj*
IconList::GetNext ()
{
	d_cur = d_cur->d_next;
	return (d_cur);
}


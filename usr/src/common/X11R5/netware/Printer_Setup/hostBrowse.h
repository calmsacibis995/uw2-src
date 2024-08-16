/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:hostBrowse.h	1.7"
/*----------------------------------------------------------------------------
 *	'hostBrowse' class definition.
 */
#ifndef HOSTBROWSE
#define	HOSTBROWSE

#include <X11/Intrinsic.h>
#include <mail/hosts.h> 
#include <mail/tree.h> 

typedef void					(*SelectCallback) (XtPointer clientData);
typedef void					(*UnselectCallback) (XtPointer clientData);

/*----------------------------------------------------------------------------
 *
 */
class MultiPList;

class hostBrowse {
public:								// Constructors and Destructors
								hostBrowse (XtAppContext	appContext,
											Widget			parent,
											int				numColumns = 3);
								~hostBrowse ();

private:							// Private Data
	MultiPList*					d_list;
	void*						d_node;				// Currently selected Node
	SelectCallback				d_selectCallback;
	UnselectCallback			d_unselectCallback;
	XtPointer					d_selectClientData;
	XtPointer					d_unselectClientData;

private:							// Private Methods
	void						select (char*	item,
										int		listNum,
										void*	data);
	static void					selectCallback (XtPointer	mlp,
												char*		item,
												int			listNum,
												XtPointer	data);
	static void					unselectCallback (XtPointer mlp, int listNum);
	void						loadList (void* list, int state);
	static void					loadListCallback (void*			list,
												  hostBrowse*	ptr,
												  int			state);

public:								// Public Interface Methods
	void						unselect (int listNum);
	inline int					isLeaf ();
	const char*					getPathName ();
	const char*					getLeafName ();
	void						setSelectCallback (SelectCallback	callback,
												   XtPointer		data = 0);
	void						setUnselectCallback (SelectCallback	callback,
													 XtPointer		data = 0);
	int							checkAddr (char* systemName);
};

/*----------------------------------------------------------------------------
 *	Is the currently selected node a leaf.
 */
inline int
hostBrowse::isLeaf ()
{
	return (!nodeIsInternal (d_node));
}

#endif	// HOSTBROWSE

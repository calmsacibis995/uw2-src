/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:fileSelect.h	1.3"
/*----------------------------------------------------------------------------
 *
 */
#ifndef FILESELECT
#define FILESELECT

#include <stream.h>

#include "BasicComponent.h"
#include "MultiPList.h"

typedef void					(*UpdateCallback) (XtPointer clientData);

/*----------------------------------------------------------------------------
 *
 */
class MultiPList;

class fileSelect {
public:								// Constructors/Destructors
								fileSelect (Widget parent, char* startPath);
								~fileSelect ();

private:							// Private Data
	MultiPList*					d_list;
	Widget						d_widget;
	Widget						d_pathWidget;
	char*						d_path;

	char**						d_names;
	int							d_numNames;
	int							d_sizeNames;

	UpdateCallback				d_updateCallback;
	XtPointer					d_clientData;

private:							// Private Methods
	void						changePath (char* path = 0);
	int							updatePath (char*	path,
											int		list = 0,
											char*	item = 0);
	void						select (char* item, int pos);
	void						newPath ();

	int							growNames (int increment);
	void						deleteName (int pos);
	void						updateName ();

	static void					selectCallback (XtPointer,
												char*,
												int,
												XtPointer);
	static void					newPathCallback (Widget,
												 XtPointer,
												 XtPointer);

public:								// Public Interface Methods
	int							appendName (const char* name);
	const char*					getName ();
	char*						getPathName ();
	inline void					manage ();
	inline void					unmanage ();
	inline Widget				widget ();
	void						setUpdateCallback (UpdateCallback	callback,
												   XtPointer		data = 0);
};

/*----------------------------------------------------------------------------
 *
 */
inline void
fileSelect::manage ()
{
	XtManageChild (d_widget);
};

inline void
fileSelect::unmanage ()
{
	XtUnmanageChild (d_widget);
};

inline Widget
fileSelect::widget ()
{
	return (d_widget);
};

#endif	// FILESELECT

#ident	"@(#)prtsetup2:hostBrowse.C	1.7"
/*----------------------------------------------------------------------------
 *	Source for 'hostBrowse' class.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream.h>
#include <netdir.h>

#include "BasicComponent.h"
#include "MultiPList.h"
#include "hostBrowse.h"

extern "C" void					connSetApplicationContext (XtAppContext);

/*----------------------------------------------------------------------------
 *	Constructors/Destructors.
 */
hostBrowse::hostBrowse (XtAppContext appContext, Widget parent, int numColumns)
{
	void*						root;

	d_node = 0;
	d_selectCallback = 0;
	d_unselectCallback = 0;

	connSetApplicationContext (appContext);

	if (!(d_list = new MultiPList (parent,
								   numColumns,
								   &hostBrowse::selectCallback,
								   (void*)this))) {
#ifdef DEBUG
		cerr << "Constructor Failed" << endl;
#endif
		exit (0);					//	THIS SHOULD BE DONE DIFFERENTLY !!!!!!!!
	}
	d_list->SetClearCallback(0);
	d_list->SetUnSelectCallback(&hostBrowse::unselectCallback);

	if (root = hostInit (0)) {
		treeListOpen (nodeTreeList (root),
					  (void (*)())loadListCallback,
					  (void*)this);
	}
	d_list->DisplayLists ();
}

hostBrowse::~hostBrowse ()
{
	delete (d_list);
}

/*----------------------------------------------------------------------------
 *
 */
void
hostBrowse::setSelectCallback (SelectCallback func, XtPointer data)
{
	d_selectCallback = func;
	d_selectClientData = data;
}

/*----------------------------------------------------------------------------
 *
 */
void
hostBrowse::setUnselectCallback (SelectCallback func, XtPointer data)
{
	d_unselectCallback = func;
	d_unselectClientData = data;
}

/*----------------------------------------------------------------------------
 *	Return the currently selected host name.
 */
const char* 
hostBrowse::getPathName ()
{
	if (isLeaf ()) {
		return (hostName (nodeData (d_node)));
	}
	return (0);
}

/*----------------------------------------------------------------------------
 *	Return the currently selected leaf node name.
 */
const char* 
hostBrowse::getLeafName ()
{
	if (isLeaf ()) {
		return (nodeName (d_node));
	}
	return (0);
}

/*----------------------------------------------------------------------------
 *	MultiPList Select callback.
 */
void 
hostBrowse::select (char* item, int listNum, void* data)
{
	void*						treePt;

	d_node = data;
	if (nodeIsInternal (d_node)) {
		d_list->ClearLists (listNum + 1);
		treePt = nodeTreeList (d_node);
		d_list->SetListClientData ((XtPointer)treePt);
		treeListOpen (treePt, (void (*)())loadListCallback, (void*)this);
	}

	if (d_selectCallback) {
		d_selectCallback (d_selectClientData);
	}

//	cerr << "Test (" << getPathName () << ", "
//					 << getLeafName () << ", "
//					 << isLeaf () << ")" << endl;
}

void 
hostBrowse::selectCallback (XtPointer	mlp,
							char*		item,
							int			listNum,
							XtPointer	data)
{
	MultiPList*					list = (MultiPList*)mlp;
	hostBrowse*					thisPtr;

	thisPtr = (hostBrowse*)list->GetObjClientData ();
	thisPtr->select (item, listNum, (void*)data);
}

/*----------------------------------------------------------------------------
 *	MultiPList unselect callback.
 */
void 
hostBrowse::unselect (int listNum)
{
	d_list->ClearLists (listNum + 1);
//	XmListDeselectAllItems (d_list->GetListWidget (listNum));

	if (d_unselectCallback) {
		d_unselectCallback (d_unselectClientData);
	}
}

void 
hostBrowse::unselectCallback (XtPointer mlp, int listNum)
{
	MultiPList*					list = (MultiPList*)mlp;
	hostBrowse*					thisPtr;

	thisPtr = (hostBrowse*)list->GetObjClientData ();
	thisPtr->unselect (listNum);
}

/*----------------------------------------------------------------------------
 *	treeListOpen callback to load the list of node names.
 */
void
hostBrowse::loadList (void* list, int state)
{
	XmString					xmStr;
	void*						nodePt;
	char*						name;
	char*						str;

	for (nodePt = treeListGetFirst (list); nodePt; nodePt = nodeNext (nodePt)) {
		if (!(name = nodeName (nodePt))) {
			continue;
		}
		if (nodeIsInternal (nodePt)) {
			if (!(str = new char[strlen (name) + 4])) {
				continue;
			}
			sprintf (str, "%s >", name);
			xmStr = XmStringCreate (str, XmSTRING_DEFAULT_CHARSET);
			delete (str);
		}
		else {
			xmStr = XmStringCreate (name, XmSTRING_DEFAULT_CHARSET);
		}

		d_list->AddListItem (xmStr, 0, nodePt);
	}
	d_list->NextList ();
	d_list->DisplayLists ();
}

void
hostBrowse::loadListCallback (void* list, hostBrowse* thisPtr, int state)
{
	thisPtr->loadList (list, state);
}

/*----------------------------------------------------------------------------
 *
 */
int
hostBrowse::checkAddr (char* systemName)
{
	struct netconfig*			config_p;
	struct nd_hostserv			hostserv;
	struct nd_addrlist*			addrlist;
	void*						handle_p = 0;
	int							result = 0;
	
	hostserv.h_host = systemName;
	hostserv.h_serv = "smtp";
	if (systemName == NULL) {
		return (0);
	}

	for (handle_p = setnetpath (), result = 0;
		 !result && (config_p = getnetpath (handle_p)) != NULL;) {
		if (!netdir_getbyname (config_p, &hostserv, &addrlist)) {
			if(addrlist->n_cnt > 0) {
				result = 1;
				break;
			}
		}
	}
	
	if (handle_p != NULL) {
		endnetpath (handle_p);
	}

	return (result);
}


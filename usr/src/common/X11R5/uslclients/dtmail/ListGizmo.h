/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:ListGizmo.h	1.5"
#endif

/*
 * ListGizmo.h
 *
 */

#ifndef _ListGizmo_h
#define _ListGizmo_h

/*
 * ListGizmo
 *
 * The \fIListGizmo\fP is used to construct a scrolled list interface
 * element consisting of a caption (optional), scrolled window, and list.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ListGizmo.h>
 * ... 
 */

typedef struct {
	XtArgVal        set;
	XtArgVal        fields;
	XtArgVal        clientData;
} ListItem;

typedef struct {
	ListItem *      items;
	int             size;           /* # of items in the list   */
	int             numFields;      /* # of fields in each item */
	XtArgVal        clientData;
} ListHead;

typedef struct _ListGizmo {
	HelpInfo *      help;            /* help information          */
	char *          name;            /* name of the widget        */
	char *          caption;         /* caption label             */
	Setting *       settings;        /* settings                  */
	char *          format;          /* list format               */
	XtArgVal        exclusive;       /* XtNexclusive              */
	int             height;          /* item height               */
	char *          font;
	ListHead *	list;            /* The flat list itself      */
	void            (*executeCB)();
	void            (*selectCB)();
	void            (*unselectCB)();
	XtCallbackRec *	limitsCB;	 /* Limits exceeded callback */
	ArgList         args;
	Cardinal        num_args;
	Widget          flatList;
} ListGizmo;

extern GizmoClassRec ListGizmoClass[];

/*
 * FIX: needed ?
 */

extern void   FreeList (ListGizmo *gizmo, ListHead * flist);
extern char * GetListField(ListGizmo *gizmo, int item);
extern void   UpdateList(ListGizmo * gizmo);

#endif /* _ListGizmo_h */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:PopupGizmo.h	1.7"
#endif

/*
 * PopupGizmo.h
 *
 */

#ifndef _PopupGizmo_h
#define _PopupGizmo_h

/*
 * PopupGizmo
 *
 * The PopupGizmo is used to construct a Popup Dialog interface
 * element.  Dialogs are constructed from the list of Gizmos presented
 * in the GizmoArray located at the top of the window.  The menubar
 * is positioned at the base of the shell and is constructed using the
 * MenuGizmo.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ... 
 */

typedef struct _PopupGizmo 
{
   HelpInfo *   help;       /* help information          */
   char *       name;       /* name of the shell         */
   char *       title;      /* title (for the wm)        */
   Gizmo        menu;       /* Pointer to menu info      */
   GizmoArray   gizmos;     /* the gizmo list            */
   int          num_gizmos; /* number of gizmos          */
   ArgList      args;       /* args applied to the shell */
   Cardinal     num_args;   /* number of args            */
   char *       error;      /* error message displayed in left footer     */
   char *       status;     /* status message displayed in right footer   */
   int          error_percent;/* percent of error message                  */
   Widget       message;    /* footer message widget     */
   Widget       shell;      /* Popup shell               */
} PopupGizmo;

extern GizmoClassRec PopupGizmoClass[];

extern void         BringDownPopup();
extern Widget       GetPopupGizmoShell();
extern void         SetPopupMessage();
extern void         SetPopupStatus();

#endif /* _PopupGizmo_h */

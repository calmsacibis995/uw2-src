/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ModalGizmo.h	1.6"
#endif

/*
 * ModalGizmo.h
 *
 */

#ifndef _ModalGizmo_h
#define _ModalGizmo_h

/*
 * ModalGizmo
 *
 * The ModalGizmo is used to construct a Modal Shell interface
 * element consisting of a text and controls in the the upper area
 * and a menubar (constructed from a MenuGizmo) placed at the bottom
 * of the shell.  The map_function of this Gizmo uses XtPopup
 * with GrabExclusive.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ModalGizmo.h>
 * ... 
 */

typedef struct _ModalGizmo 
{
   HelpInfo *  help;       /* help information                       */
   char *      name;       /* name of the shell                      */
   char *      title;      /* title of the window                    */
   MenuGizmo * menu;       /* Pointer to menu info                   */
   char *      message;    /* message for stext (Notice case)        */
   GizmoArray  gizmos;     /* gizmos for the upper area              */
   int         num_gizmos; /* number of gizmos                       */
   Widget      control;    /* control area (returned)                */
   Widget      stext;      /* static text widget (returned)          */
   Widget      shell;      /* Modal shell (returned)                 */
} ModalGizmo;

extern GizmoClassRec ModalGizmoClass[];

extern Widget   GetModalGizmoShell(ModalGizmo *);
extern void     SetModalGizmoMessage(ModalGizmo *, char *);

#endif /* _ModalGizmo_h */

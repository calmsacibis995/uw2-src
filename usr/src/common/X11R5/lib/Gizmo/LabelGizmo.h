/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:LabelGizmo.h	1.5"
#endif

#ifndef _LabelGizmo_h
#define _LabelGizmo_h

/*
 * LabelGizmo
 *
 * The LabelGizmo is used to construct a Label control area
 * element.  This gizmo allows the grouping of controls in a caption.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <LabelGizmo.h>
 * ... 
 */

typedef struct _LabelGizmo
{
   HelpInfo *   help;          /* help information               */
   char *       name;          /* name for the widget            */
   char *       caption;       /* caption label (I18N)           */
   GizmoArray   gizmos;        /* Gizmo list for this LabelGizmo */
   int          num_gizmos;    /* number of gizmos in array      */
   int          layoutType;    /* OL_FIXEDROWS, OL_FIXEDCOLS     */
   int          measure;       /* Number rows, cols              */
   ArgList      args;          /* Arg array for the controlArea  */
   int          num_args;      /* number of Args in the array    */
   Boolean      alignCaptions; /* Align captions                 */
   Widget       captionWidget; /* (return) Caption Widget        */
   Widget       controlWidget; /* (return) ControlArea Widget    */
} LabelGizmo;

extern GizmoClassRec   LabelGizmoClass[];

extern Widget      GetControlWidget();

#endif /* _LabelGizmo_h */

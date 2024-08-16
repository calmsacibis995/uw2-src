/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:NumericGiz.h	1.6"
#endif

#ifndef _NumericGiz_h
#define _NumericGiz_h

/*
 * NumericGizmo
 *
 * The NumericGizmo is used to construct a NumericText interface
 * element consisting of a caption (optional), text field, increment
 * controls, and auxiliary label (optional).
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <NumericGizmo.h>
 * ... 
 */

typedef struct _NumericGizmo 
{
   HelpInfo * help;            /* help information                         */
   char *     name;            /* name of widget                           */
   char *     caption;         /* caption                                  */
   int        min;             /* minimum value                            */
   int        max;             /* maximum value                            */
   Setting *  settings;        /* settings                                 */
   int        charsVisible;    /* number of digits to show                 */
   char *     label;           /* label (on right)                         */
   ArgList    args;            /* Arg array used for the textField Widget  */
   int        num_args;        /* number of Args in the list               */
   Widget     captionWidget;   /* (return) caption Widget                  */
   Widget     labelWidget;     /* (return) label Widget                    */
   Widget     controlWidget;   /* (return) control Widget                  */
   Widget     textFieldWidget; /* (return) textField Widget                */
   Widget     scrollbarWidget; /* (return) scrollbar Widget                */
} NumericGizmo;

extern GizmoClassRec NumericGizmoClass[];

extern int           GetNumericFieldValue(NumericGizmo *);
extern void          SetNumericFieldValue(NumericGizmo *, int);

#endif /* _NumericGiz_h */

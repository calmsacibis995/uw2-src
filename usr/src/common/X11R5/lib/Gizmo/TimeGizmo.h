/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:TimeGizmo.h	1.3"
#endif

/*
 * TimeGizmo.h
 *
 */

#ifndef _TimeGizmo_h
#define _TimeGizmo_h

/*
 * TimeGizmo
 *
 * The TimeGizmo is used to construct a text field input interface
 * element.  The text field is enclosed in a Caption Widget.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <TimeGizmo.h>
 * ... 
 */

typedef struct _TimeGizmo 
{
   HelpInfo *      help;             /* help information                */
   char *          name;             /* name for the Widget             */
   char *          caption;          /* caption laebl (I18N)            */
   char *          text;             /* text                            */
   Setting *       settings;         /* settings                        */
   ArgList         args;             /* Args used to tune the textField */
   Cardinal        num_args;         /* number of Args                  */
   Widget          captionWidget;    /* (return) Caption Widget         */
   Widget          textFieldWidget;  /* (return) TextField Widget       */
} TimeGizmo;

extern GizmoClassRec TimeGizmoClass[];

extern char * FormatTime(char * time, int *hourp, int * minp);
extern char * GetTime(PopupGizmo *shell, int item);
extern void   SetTime(PopupGizmo *shell, int item, char *text, int selected);

#endif /* _TimeGizmo_h */

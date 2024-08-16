/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:STextGizmo.h	1.7"
#endif

/*
 * STextGizmo.h
 *
 */

#ifndef _STextGizmo_h
#define _STextGizmo_h

/*
 * StaticTextGizmo
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ... 
 */

typedef struct _StaticTextGizmo
{
   HelpInfo *  help;         /* help information              */
   char *      name;         /* name of the widget            */
   char *      text;         /* text string                   */
   OlDefine    gravity;      /* text gravity                  */
   char *      font;         /* text font                     */
   Widget      widget;       /* static text widget (returned) */
} StaticTextGizmo;

extern GizmoClassRec StaticTextGizmoClass[];

#endif /* _STextGizmo_h */

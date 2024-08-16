/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)notice:Modal.h	1.1"
#endif

#ifndef _Modal_h
#define _Modal_h

#include <X11/Shell.h>		/* include superclasses's header file */

/*
 * OPEN LOOK(TM) Modal Shell Widget
 */

/* Class record pointer */
extern WidgetClass modalShellWidgetClass;

/* C Widget type definition */
typedef struct _ModalShellClassRec	*ModalShellWidgetClass;
typedef struct _ModalShellRec		*ModalShellWidget;

#endif /* _Modal_h */

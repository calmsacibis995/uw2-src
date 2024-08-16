/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/wsm.h	1.38"

#ifndef _WSM_H
#define _WSM_H

#define WSM			"Workspace Manager"
#define DISPLAY			XtDisplay(InitShell)
#define SCREEN			XtScreen(InitShell)
#define ROOT			RootWindowOfScreen(SCREEN)
#define HELP(name)		name
#define MOD1			"Alt"

extern Widget		InitShell;
extern Widget		handleRoot;
extern Widget		workspaceMenu;
extern Widget		programsMenu;

extern char *		GetPath(char *);
extern void		FooterMessage(Widget, String, /* OlDefine, */ Boolean);
extern void		WSMExit( void );
extern int		ExecCommand(char * );
extern void		RefreshCB ( Widget, XtPointer, XtPointer);
extern void		CreateWorkSpaceMenu(Widget, Widget *, Widget *	);
extern void		RestartWorkspaceManager( void );
extern void		TouchPropertySheets( void );

#endif

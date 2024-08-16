/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:panel/llib-lpanel.c	1.5"
/*LINTLIBRARY*/

#include <panel.h>

int bottom_panel (panel) PANEL *panel; {return OK;}
int hide_panel (panel) PANEL *panel; {return OK;}
int del_panel (panel) PANEL *panel; {return OK;}
WINDOW *panel_window (panel) PANEL *panel; {return (WINDOW *) 0;}
char *panel_userptr (panel) PANEL *panel; {return (char *)0;}
int set_panel_userptr (panel, ptr) PANEL *panel; char *ptr; {return OK;}
PANEL *panel_above (panel) PANEL *panel; {return (PANEL *) 0;}
PANEL *panel_below (panel) PANEL *panel; {return (PANEL *) 0;}
int panel_hidden (panel) PANEL *panel; {return TRUE;}
int move_panel (panel, starty, startx) PANEL *panel; int starty, startx; {return OK;}
PANEL *new_panel (window) WINDOW *window; {return (PANEL *) 0;}
int show_panel (panel) PANEL *panel; {return OK;}
int replace_panel (panel, window) PANEL	*panel; WINDOW	*window; {return OK;}
int top_panel (panel) PANEL *panel; {return OK;}
void update_panels () {}

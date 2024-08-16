/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	CONFIG_H
#define CONFIG_H
#ident	"@(#)debugger:gui.d/common/config.h	1.5"

#include "Panes.h"
#include "UI.h"
#include "Button_bar.h"

class Menu_bar_table;
class Button_bar_table;

struct Pane_descriptor
{
	const char		*name;
	Pane_type		type;
	int			nlines;
	int			ncolumns;
	const Menu_bar_table	*menu_table;
};

#define W_AUTO_POPUP	0x1
#define W_HAS_STATUS	0x2
#define W_HAS_EVENT	0x4
#define W_HAS_COMMAND	0x8
#define W_BUTTONS_BOTTOM	0x10

struct Window_descriptor
{
	LabelId			label;
	Pane_descriptor		**panes;
	const Menu_bar_table	*menu_table;
	int			nmenus;
	int			npanes;
	Button_bar_table	**button_table;
	short			nbuttons;
	unsigned short		flags;		
	const char		*name;
};

extern	Window_descriptor	*make_descriptors();
extern	Window_descriptor	*window_descriptor;
extern	Window_descriptor	second_source_wdesc;

extern	int			windows_per_set;
extern	int			cmd_panes_per_set;
extern	int			max_rows;

extern Window_descriptor	*read_user_file();
extern const Button_bar_table	*get_button_desc(CButtons);
extern const Pane_descriptor	*get_pane_desc(Pane_type);
extern const Window_descriptor	*get_win_desc(Pane_type);

#endif // CONFIG_H

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_DISPP_H
#define	_TEXT_DISPP_H
#ident	"@(#)debugger:libol/common/Text_dispP.h	1.6"

// toolkit specific members of the Text_display class
// included by ../../gui.d/common/Text_area.h

//  gc_base and gc_stop are Graphics Contexts - information about foreground,
//	background, etc. needed to draw the arrow and stop sign
// breaks is an array with one bit per line in the displayed text, with the
//	bit on if there is a breakpoint on that line

#define	TEXT_DISPLAY_TOOLKIT_SPECIFICS	\
private:				\
	int		current_line;	\
	int		last_line;	\
	const char	*search_expr;	\
	void		*compiled_expr;	\
	char		*breaks;	\
	Boolean		is_source;	\
	Boolean		empty;		\
	Boolean		user_selection;	\
					\
	int		top_margin;	\
	int		x_arrow;	\
	int		x_stop;		\
	int		left_margin;	\
	int		font_height;	\
	GC		gc_base;	\
	GC		gc_stop;	\
					\
	void		draw_number(int line, int top_line);	\
	void		draw_arrow(int line, int lower_bound);	\
	void		clear_arrow(int line, int lower_bound);	\
	void		draw_stop(int line, int lower_bound);	\
	void		pane_bounds(int &lower, int &upper);	\
					\
	void		finish_setup();	\
	void		set_gc();	\
					\
public:					\
	void		set_selection(int start, int end); \
	void		set_selection_size(int n)	{ selection_size = n; } \
	void		fix_margin(XRectangle *); \
	void		redisplay(Boolean);

#endif	// _TEXT_DISPP_H

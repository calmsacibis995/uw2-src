/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TABLEP_H
#define	_TABLEP_H
#ident	"@(#)debugger:libol/common/TableP.h	1.9"

// toolkit specific members of the Table class
// included by ../../gui.d/common/Table.h

#include <stdarg.h>
#include "Vector.h"

struct Item_data;	// defined in Table.C
struct Column_data;	// defined in Table.C

#define TABLE_TOOLKIT_SPECIFICS \
private:						\
	Widget		input;				\
	char		*format;			\
	Item_data	*item_data;			\
	Item_data	*old_data;			\
	Item_data	*blank_data;			\
	Vector		vector;				\
	Column_data	*column_spec;			\
	Boolean		sensitive;			\
	Boolean		wrapped;			\
	int		font_width;			\
	Pixmap		hand_pm;			\
	Pixmap		solid_pm;			\
	Pixmap		pin_pm;				\
	int		delete_start;			\
	int		delete_total;			\
	int		overflow;			\
							\
	char		*make_format(Boolean wrap);			\
	void		fix_column_spec(XFontStruct *, OlFontList *);	\
	void		make_title(XFontStruct *,OlFontList *, Widget);	\
	void		set_row(Item_data *, va_list);			\
	void		make_glyphs(Widget);				\
	void		update_glyph(char **, Boolean, Glyph_type);	\
	void		cleanup(Item_data *, int total);		\
	void		overflow_handler(Item_data *& list);		\
									\
public:									\
	void		update_glyphs(int row, Boolean highlight);	\
	void		set_overflow(int new_size);

#endif	// _TABLEP_H

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_AREAP_H
#define	_TEXT_AREAP_H
#ident	"@(#)debugger:libol/common/Text_areaP.h	1.4"

// toolkit specific members of the Text_area class
// included by ../../gui.d/common/Text_area.h

struct _TextBuffer;

// The constructor declared here is called from the Text_display constructor.
// It is declared here instead of in ../../gui.d/common/Text_area.h because
// the way the two classes relate is really toolkit-specific

#define TEXT_AREA_TOOLKIT_SPECIFICS		\
protected:					\
	Widget		text_area;		\
	XFontStruct	*font;			\
	Position	*tab_table;		\
	_TextBuffer	*textbuf;		\
	char		*string;		\
	int		selection_start;	\
	int		selection_size;		\
						\
	void		setup_tab_table();	\
						\
public:						\
			Text_area(Component *, const char *name, \
				Command_sender *creator, \
				Callback_ptr select_cb, Help_id help_msg);	\
						\
	virtual void	set_selection(int start, int end);

#endif	// _TEXT_AREAP_H

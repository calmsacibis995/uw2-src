/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _LABEL_H_

#ident	"@(#)debugger:gui.d/common/Label.h	1.1"

// provide internationalized label support for framework
// classes

#include "gui_label.h"

struct Label {
	short		catindex;
	unsigned char	local_set;
	const char	*label;
};

// table of labels
class LabelTab {
	Label		*labtab;
	int		cat_available;
public:
			LabelTab();
			~LabelTab() {}
	void		init();
	const char	*get_label(LabelId);
};

extern LabelTab		labeltab;

#define _LABEL_H_
#endif

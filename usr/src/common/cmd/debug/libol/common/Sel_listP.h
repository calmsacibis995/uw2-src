/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SELECTIONP_H
#define	_SELECTIONP_H
#ident	"@(#)debugger:libol/common/Sel_listP.h	1.3"

// toolkit specific members of the Selection_list class
// included by ../../gui.d/common/Selection.h

#define SELECTION_LIST_TOOLKIT_SPECIFICS	\
private:					\
	Widget		list;			\
	char		***item_data;		\
	int		total_items;		\
	int		new_size;		\
	int		columns;		\
	char		**pointers;		\
	Boolean		overflow;		\
						\
	void		allocate_pointers(int rows); \
	void		handle_overflow();	\
						\
public:						\
	void		set_overflow(int size) { new_size = size; overflow = TRUE; }

#endif	// _SELECTIONP_H

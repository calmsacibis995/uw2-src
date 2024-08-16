/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_BOXES_H
#define	_BOXES_H
#ident	"@(#)debugger:gui.d/common/Boxes.h	1.7"

#include "Component.h"
#include "List.h"
#include "BoxesP.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define BOX	Box
#else
#define BOX
#endif

enum Box_type {
	Box_packed,
	Box_divided,
	Box_expansion,
};

class Box : public Component
{
	BOX_TOOLKIT_SPECIFICS

protected:
	Box_type	type;
	List		children;

public:
			Box(Component *parent, const char *name,
				Orientation, Box_type,
				Help_id help_msg = HELP_none);
	virtual		~Box();

	virtual void	add_component(Component *, Boolean elastic = FALSE);
	Box_type	get_type() { return type; }
};
	
// Components in a packed box are placed one after another in the major dimension
// (horizontal or vertical).  Components do not change size or position if the
// box is resized in the major dimension; they are resized in minor dimension
// in all box types.
class Packed_box : public Box
{
	PACKBOX_TOOLKIT_SPECIFICS

public:
			Packed_box(Component *parent, const char *name,
				Orientation, Help_id help_msg = HELP_none);
			~Packed_box() {}

	void	add_component(Component *, Boolean elastic = FALSE);
};

// One or more children of an Expansion_box are elastic components.
// When the box is resized in the major dimension, the elastic components
// are resized.  All others are unchanged
class Expansion_box : public Box
{
	EXPANSIONBOX_TOOLKIT_SPECIFICS

public:
			Expansion_box(Component *parent, const char *name,
				Orientation, Help_id help_msg = HELP_none);
			~Expansion_box() {}

	void	add_component(Component *, Boolean elastic = FALSE);
};

// A Divided_box is vertical only.  The user may re-apportion the panes
// within the box by clicking and dragging sashes
class Divided_box : public Box
{
	DIVIDEDBOX_TOOLKIT_SPECIFICS

public:
			Divided_box(Component *parent, const char *name,
				Boolean handles = TRUE,
				Help_id help_msg = HELP_none);
			~Divided_box() {}

	void	add_component(Component *, Boolean elastic = FALSE);
};

#endif	// _BOXES_H

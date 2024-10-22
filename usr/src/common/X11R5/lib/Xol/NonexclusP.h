/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)nonexclus:NonexclusP.h	1.19"
#endif

/* 
 * Author:	Karen S. Kendler
 * Date:	16 August 1988
 * File:	NonexclusP.h - Private definitions for Nonexclusives widget
 *	Copyright (c) 1989 AT&T		
 *
 */

#ifndef _OlNonexclusivesP_h
#define _OlNonexclusivesP_h

#include <Xol/ManagerP.h>	/* include superclasses' header */
#include <Xol/Nonexclusi.h>

/* New fields for the Nonexclusives widget class record */

typedef struct _NonexclusivesClass {
    char no_class_fields;		/* Makes compiler happy */
} NonexclusivesClassPart;

   /* Full class record declaration */
typedef struct _NonexclusivesClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    NonexclusivesClassPart	nonexclusives_class;
} NonexclusivesClassRec;

extern NonexclusivesClassRec nonexclusivesClassRec;

		    /* New fields for the Nonexclusives widget record: */
typedef struct {

		   				/* fields for resources */
    OlDefine		layout;			/* public */
    int			measure;
    Boolean		recompute_size;

    Boolean		is_default;		/* private */
    XtPointer		default_data;
    Widget		preview;	
    Boolean		trigger;	
    int			shell_behavior;
    XtCallbackList	postselect;

    int			reset_default;

					/* fields for internal management */ 
    int			class_children;
    Widget		default_child;
    Dimension		max_height;
    Dimension		max_width;
    Dimension		normal_height;
    Dimension		normal_width;

} NonexclusivesPart;

/*    XtEventsPtr eventTable;*/


   /* Full widget declaration */
typedef struct _NonexclusivesRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    NonexclusivesPart	nonexclusives;
} NonexclusivesRec;

#endif /* _OlNonexclusivesP_h */

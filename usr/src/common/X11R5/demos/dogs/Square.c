/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:dogs/Square.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: Square.c,v $ $Revision: 1.5 $ $Date: 92/03/13 15:34:22 $"
#endif
#endif

/*****************************************************************************
*
*  Square.c - widget source file
*
******************************************************************************/
	
#include <stdio.h>
#include <Xm/Xm.h>
#include <Mrm/MrmPublic.h>
#include <Xm/BulletinBP.h>
#include <SquareP.h>

#define Width(w) XmField((w),square_offsets,Core,width,Dimension)
#define Height(w) XmField((w),square_offsets,Core,height,Dimension)
#define BorderWidth(w) XmField((w),square_offsets,Core,border_width,Dimension)
#define MajorDimension(w) XmField((w),square_offsets,Square,major_dimension,int)

#define MakeSquare(w) XmConstraintField((w),square_constraint_offsets, \
			Square,make_square, Boolean)

static void ClassInitialize();
static void Initialize();
static Boolean SetValues();
static void ConstraintInitialize();
static Boolean ConstraintSetValues();

static XmPartResource resources[] = {
    {
    SquareNmajorDimension, SquareCMajorDimension, XmRInt, sizeof(int),
    XmPartOffset(Square,major_dimension), XmRImmediate, (caddr_t)SquareWIDTH
    }
};

static XmPartResource constraints[] = {
   {
    SquareNmakeSquare, SquareCMakeSquare, XmRBoolean, sizeof(Boolean),
    XmConstraintPartOffset(Square,make_square),
    XmRImmediate, (caddr_t)False
   }
};


externaldef(squareclassrec) SquareClassRec squareClassRec =
{
   {                    /* core_class fields    */
      (WidgetClass) &xmBulletinBoardClassRec,   /* superclass   */
      "Square",                 /* class_name           */
      sizeof(SquarePart),       /* widget_size          */
      ClassInitialize,          /* class_initialize     */
      NULL,                     /* class init part proc */
      False,                    /* class_inited         */
      Initialize,               /* initialize           */
      NULL,                     /* initialize_notify    */
      XtInheritRealize,         /* realize              */
      NULL,                     /* actions              */
      0,                        /* num_actions          */
      (XtResourceList)resources,/* resources            */
      XtNumber(resources),      /* num_resources        */
      NULLQUARK,                /* xrm_class            */
      False,                    /* compress_motion      */
      XtExposeCompressMaximal,  /* compress_exposure    */
      False,                    /* compress_enterleave  */
      False,                    /* visible_interest     */
      NULL,                     /* destroy              */
      XtInheritResize,          /* resize               */
      XtInheritExpose,          /* expose               */
      SetValues,                /* set_values           */
      NULL,                     /* set_values_hook      */
      XtInheritSetValuesAlmost, /* set_values_almost    */
      NULL,                     /* get_values_hook      */
      NULL,                     /* accept_focus         */
      XtVersionDontCheck,       /* version              */
      NULL,                     /* callback_private     */
      XtInheritTranslations,    /* tm_table             */
      XtInheritQueryGeometry,   /* Query Geometry proc  */
      NULL,                     /* disp accelerator     */
      NULL,                     /* extension            */    
   },

   {                    /* composite_class fields */
      XtInheritGeometryManager, /* geometry_manager       */
      XtInheritChangeManaged,   /* change_managed         */
      XtInheritInsertChild,     /* insert_child           */
      XtInheritDeleteChild,     /* delete_child           */
      NULL,                     /* extension              */
   },

   {                    /* constraint_class fields */
      (XtResourceList)constraints,  /* constraint resource     */
      XtNumber(constraints),        /* number of constraints   */
      sizeof(SquareConstraintPart), /* size of constraint      */
      ConstraintInitialize,         /* initialization          */
      NULL,                         /* destroy proc            */
      ConstraintSetValues,          /* set_values proc         */
      NULL,                         /* extension               */
   },

   {                    /* manager_class fields   */
      XtInheritTranslations,        /* translations           */
      NULL,                         /* syn_resources          */
      0,                            /* num_syn_resources      */
      NULL,                         /* syn_cont_resources     */
      0,                            /* num_syn_cont_resources */
      XmInheritParentProcess,       /* parent_process         */
      NULL,                         /* extension              */
   },

   {                    /* bulletin_board_class fields */
      FALSE,                        /* always_install_accelerators */
      NULL,                         /* geo_matrix_create  */
      XtInheritFocusMovedProc,      /* focus_moved_proc   */
      NULL,                         /* extension          */
   },

   {                    /* square_class fields  */
      NULL,                         /* extension          */
   }
};

externaldef(squarewidgetclass) WidgetClass 
	squareWidgetClass = (WidgetClass) &squareClassRec;

static XmOffsetPtr square_offsets;		/* Part offsets table */
static XmOffsetPtr square_constraint_offsets;	/* Constraint offsets table */

/**********************************************************************
 *
 * SquareCreate - Convenience routine, used by Uil/Mrm.
 *
 *********************************************************************/

Widget SquareCreate(parent, name, arglist, nargs)
    Widget parent;
    char *name;
    Arg *arglist;
    int nargs;
{
    return(XtCreateWidget (name, squareWidgetClass, parent, arglist, nargs));
}

/**********************************************************************
 *
 * SquareMrmInitialize - register Square widget class with Mrm
 *
 *********************************************************************/

int SquareMrmInitialize()
{
    return(MrmRegisterClass (MrmwcUnknown, "Square", "SquareCreate",
			     SquareCreate, (WidgetClass)&squareClassRec));
}

/**********************************************************************
 *
 * Class methods
 *
 *********************************************************************/

static void ClassInitialize()
{
    XmResolveAllPartOffsets(squareWidgetClass, &square_offsets,
				&square_constraint_offsets);
}

static void Initialize(req, new)
    SquareWidget req;
    SquareWidget new;
{
    if (MajorDimension(new) != SquareWIDTH &&
	MajorDimension(new) != SquareHEIGHT) {
	XtWarning("Square: invalid majorDimension");
	MajorDimension(new) = SquareWIDTH;
    }
}

static Boolean SetValues(curr, req, new)
    SquareWidget curr;
    SquareWidget req;
    SquareWidget new;
{
    if (MajorDimension(new) != SquareWIDTH &&
	MajorDimension(new) != SquareHEIGHT) {
	XtWarning("Square: invalid majorDimension");
	MajorDimension(new) = MajorDimension(curr);
    }
    return (False);
}

static void ConstraintInitialize (req, new)
    Widget req;
    Widget new;
{
    Dimension m;

    if(MakeSquare(new) == True) {
        if (MajorDimension(XtParent(new))==SquareWIDTH)
            m = Width(new);
        else
            m = Height(new);

	XtResizeWidget(new, m, m, BorderWidth(new));
    }
}

static Boolean ConstraintSetValues (old, ref, new)
    Widget old;
    Widget ref;
    Widget new;
{
    Boolean redraw = False;

    if (MakeSquare(new) != MakeSquare(old)) {
	if(MakeSquare(new)==True) {
            if (MajorDimension(XtParent(new))==SquareWIDTH)
        	Height(new) = Width(new);
            else
        	Width(new) = Height(new);
	}
	else {
	    XtWidgetGeometry gi;
	    XtWidgetGeometry gp;

            if (MajorDimension(XtParent(new))==SquareWIDTH)
        	Height(new) = Height(new)/2;
            else
        	Width(new) = Width(new)/2;

	    gi.request_mode = CWWidth | CWHeight;
	    gi.width = Width(new);
	    gi.height = Height(new);
	    if (XtQueryGeometry(new, &gi, &gp) == XtGeometryAlmost) {
		if (gp.request_mode && CWWidth != 0) Width(new) = gp.width;
		if (gp.request_mode && CWHeight != 0) Height(new) = gp.height;
	    }
	}
	redraw = True;
    }
    return (redraw);
}


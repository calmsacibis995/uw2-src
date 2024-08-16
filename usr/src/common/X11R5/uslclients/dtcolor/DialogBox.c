/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:DialogBox.c	1.2"
/* static char rcsid[] = 
	"$Header: DialogBox.c,v 1.1 93/08/17 16:30:07 xbuild Exp $";
*/
/**---------------------------------------------------------------------
***	
***	file:		DialogBox.c
***
***	project:	Motif Widgets
***
***	description:	Source code for DtDialogBox class.
***	
***
***			(c) Copyright 1990 by Hewlett-Packard Company.
***
***
***-------------------------------------------------------------------*/


/*-------------------------------------------------------------
**	Include Files
*/

#include <stdio.h>
#include <Xm/DialogS.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <DialogBoxP.h>
#include <MacrosP.h>
#include <DtMsgsP.h>

/*-------------------------------------------------------------
**	Public Interface
**-------------------------------------------------------------
*/

WidgetClass	dtDialogBoxClass;

#ifdef _NO_PROTO

extern Widget DtCreateDialogBox() ;
extern Widget DtCreateDialogBoxDialog() ;
extern Widget DtDialogBoxGetButton() ;
extern Widget DtDialogBoxGetWorkArea() ;

#else

extern Widget DtCreateDialogBox( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget DtCreateDialogBoxDialog( 
                        Widget ds_p,
                        String name,
                        ArgList db_args,
                        Cardinal db_n) ;
extern Widget DtDialogBoxGetButton( 
                        Widget w,
                        Cardinal pos) ;
extern Widget DtDialogBoxGetWorkArea( 
                        Widget w) ;

#endif /* _NO_PROTO */
#define Max(x, y)    (((ushort_t)(x) > (ushort_t)(y)) ? (x) : (y))

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Destroy() ;
static Boolean SetValues() ;
static void ChangeManaged() ;
static void InsertChild() ;
static void DeleteChild() ;
static void ConstraintInitialize() ;
static void GetSize() ;
static void CreateChildren() ;
static void ButtonCallback() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget request_w,
                        Widget new_w) ;
static void Destroy( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget current_w,
                        Widget request_w,
                        Widget new_w) ;
static void InsertChild( 
                        Widget child) ;
static void DeleteChild( 
                        Widget child) ;
static void ConstraintInitialize( 
                        Widget request,
                        Widget new) ;
static void GetSize( 
                        Widget manager,
                        Dimension wa_w,
                        Dimension wa_h,
                        Dimension bp_w,
                        Dimension bp_h,
                        Dimension *w,
                        Dimension *h) ;
static void CreateChildren( 
                        DtDialogBoxWidget new,
                        Widget top_widget,
                        Widget bottom_widget) ;
static void ButtonCallback( 
                        Widget g,
                        XtPointer client_data,
                        XtPointer call_data) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*-------------------------------------------------------------
**	Forward Declarations
*/

#define OFFSET		6
#define CENTER_POS	50
#define LEFT_POS	50

#define WARN_CHILD_TYPE		_DtMsgDialogBox_0000
#define WARN_BUTTON_CHILD	_DtMsgDialogBox_0001

extern void 	_DtRegisterNewConverters ();


/*-------------------------------------------------------------
**	Translations and Actions
*/



/*-------------------------------------------------------------
**	Resource List
*/

/*	Define offset macros.
*/
#define DB_Offset(field) \
	XtOffset (DtDialogBoxWidget, dialog_box.field)
#define DBC_Offset(field) \
	XtOffset (DtDialogBoxConstraintPtr, dialog_box_constraint.field)

/*	Core Resources.
*/
static XtResource resources[] = 
{
	{
		XmNminimizeButtons,
		XmCMinimizeButtons, XmRBoolean, sizeof (Boolean),
		DB_Offset (minimize_buttons), XmRImmediate, (XtPointer) False
	},
	{
		XmNbuttonCount,
		XmCButtonCount, XmRCardinal, sizeof (Cardinal),
		DB_Offset (button_count), XmRImmediate, (XtPointer) 4
	},
	{
		XmNbuttonLabelStrings,
		XmCButtonLabelStrings, XmRXmStringTable, sizeof (XtPointer),
		DB_Offset (button_label_strings), XmRStringTable, NULL
	},
	{
		XmNcallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		DB_Offset (callback), XmRImmediate, (XtPointer) NULL
	}
};

/*	Constraint Resources
*/
static XtResource constraints[] =
{
	{
		XmNchildType,
		XmCChildType, XmRChildType, sizeof (unsigned char),
		DBC_Offset (child_type), XmRImmediate, (XtPointer) XmWORK_AREA
	}
};

#undef	DB_Offset
#undef	DBC_Offset



/*-------------------------------------------------------------
**	Class Record
*/
DtDialogBoxClassRec dtDialogBoxClassRec =
{
/*	Core Part
*/
	{	
		(WidgetClass) &xmFormClassRec,	/* superclass	*/
		"DtDialogBox",			/* class_name		*/
		sizeof (DtDialogBoxRec),	/* widget_size		*/
		ClassInitialize,		/* class_initialize	*/
		ClassPartInitialize,		/* class_part_initialize*/
		False,				/* class_inited		*/
		(XtInitProc) Initialize,	/* initialize		*/
		NULL,				/* initialize_hook	*/
		XtInheritRealize,		/* realize		*/
		NULL,				/* actions		*/
		0,				/* num_actions		*/
		resources,			/* resources		*/
		XtNumber (resources),		/* num_resources	*/
		NULLQUARK,			/* xrm_class		*/
		True,				/* compress_motion	*/
		False,				/* compress_exposure	*/
		True,				/* compress_enterleave	*/
		False,				/* visible_interest	*/	
		Destroy,		 	/* destroy		*/	
		XtInheritResize,		/* resize		*/
		XtInheritExpose,		/* expose		*/	
		(XtSetValuesFunc) SetValues,	/* set_values		*/	
		NULL,				/* set_values_hook	*/
		XtInheritSetValuesAlmost,	/* set_values_almost	*/
		NULL,				/* get_values_hook	*/
		NULL,				/* accept_focus		*/	
		XtVersion,			/* version		*/
		NULL,				/* callback private	*/
		XtInheritTranslations,		/* tm_table		*/
		XtInheritQueryGeometry,		/* query_geometry	*/
		NULL,				/* display_accelerator	*/
		NULL,				/* extension		*/
	},

/*	Composite Part
*/
	{
		XtInheritGeometryManager,	/* geometry_manager	*/
		XtInheritChangeManaged,		/* change_managed	*/
		InsertChild,			/* insert_child		*/
		DeleteChild,			/* delete_child		*/
		NULL,				/* extension		*/
	},

/*	Constraint Part
*/
	{
		constraints,			/* constraint_resources	*/
		XtNumber (constraints),		/* num_constraint_resources */
		sizeof (DtDialogBoxConstraintRec),/* constraint_record	*/
		(XtInitProc) ConstraintInitialize,/* constraint_initialize */
		NULL,				/* constraint_destroy	*/
		NULL,				/* constraint_set_values */
		NULL,				/* extension		*/
	},

/*	XmManager Part
*/
	{
		XtInheritTranslations,		/* default_translations	*/
		NULL,				/* get_resources	*/
		NULL,				/* num_get_resources	*/
		NULL,				/* get_cont_resources	*/
		0,				/* num_get_cont_resources */
		XmInheritParentProcess,		/* parent_process	*/
		NULL,				/* extension		*/
	},

/*	XmBulletinBoard Part
*/
	{
		True,				/* always_install_accelerators*/
		NULL,				/* geo_matrix_create	*/
		XmInheritFocusMovedProc,	/* focus_moved_proc	*/
		NULL,				/* extension		*/
	},

/*	XmForm Part
*/
	{
		NULL,				/* extension		*/
	},

/*	DtDialogBox Part
*/
	{
(XtWidgetProc)	CreateChildren,			/* create_children	*/
		NULL,				/* configure_children	*/
		GetSize,			/* get_size		*/
		NULL,				/* button_callback	*/
		NULL,				/* extension		*/
	}

};

WidgetClass dtDialogBoxWidgetClass = (WidgetClass) &dtDialogBoxClassRec;



/*-------------------------------------------------------------
**	Private Procs
**-------------------------------------------------------------
*/


/*-------------------------------------------------------------
**	Action Procs
**-------------------------------------------------------------
*/


/*-------------------------------------------------------------
**	Core Procs
**-------------------------------------------------------------
*/

/*-------------------------------------------------------------
**	ClassInitialize
**		Initialize widget class.
*/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
	_DtRegisterNewConverters ();
}



/*-------------------------------------------------------------
**	ClassPartInitialize
**		Initialize widget class part.
*/
static void
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidgetClass	mc =	(DtDialogBoxWidgetClass) wc;
	DtDialogBoxWidgetClass	super =	(DtDialogBoxWidgetClass)
						C_Superclass (wc);

	mc->bulletin_board_class.always_install_accelerators = True;

/*	Propagate class procs for subclasses.
*/
	if (C_CreateChildren (mc) == (XtWidgetProc) _XtInherit)	
		C_CreateChildren (mc) = C_CreateChildren (super);

	if (C_ConfigureChildren (mc) == (XtWidgetProc) _XtInherit)	
		C_ConfigureChildren (mc) = C_ConfigureChildren (super);

#ifdef _NO_PROTO
	if (C_GetSize (mc) == (XtWidgetProc) _XtInherit)	
		C_GetSize (mc) = C_GetSize (super);
#else
	if (C_GetSize (mc) == (DlgGetSizeProc) _XtInherit)	
		C_GetSize (mc) = C_GetSize (super);
#endif

	if (C_ButtonCallback (mc) == (XtCallbackProc) _XtInherit)	
		C_ButtonCallback (mc) = C_ButtonCallback (super);

}



/*-------------------------------------------------------------
**	Initialize
**		Initialize a new widget instance.
*/
static void 
#ifdef _NO_PROTO
Initialize( request_w, new_w )
        Widget request_w ;
        Widget new_w ;
#else
Initialize(
        Widget request_w,
        Widget new_w )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	new	= (DtDialogBoxWidget) new_w;



	if (M_ButtonCount (new) > 0)
		M_Button (new) = (Widget *)
			XtMalloc (sizeof (Widget) * M_ButtonCount (new));
	else
		M_Button (new) = (Widget *)NULL;

	M_WorkArea (new) = NULL;
	M_CreateChildren ((Widget)new); 

	M_ButtonLabelStrings (new) = NULL;

	M_MarginWidth (new) = M_ShadowThickness (new);
	M_MarginHeight (new) = M_ShadowThickness (new);
}


/*-------------------------------------------------------------
**	Destroy
**		Release resources allocated for widget instance.
*/
static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr = (DtDialogBoxWidget) w;

/*	Free button pointers.
*/
	if (M_Button (mgr) != NULL)
		XtFree ((char *)M_Button (mgr));
}



/*-------------------------------------------------------------
**	SetValues
**		Handle changes in resource data.
*/
static Boolean 
#ifdef _NO_PROTO
SetValues( current_w, request_w, new_w )
        Widget current_w ;
        Widget request_w ;
        Widget new_w ;
#else
SetValues(
        Widget current_w,
        Widget request_w,
        Widget new_w )
#endif /* _NO_PROTO */
{

	Boolean redraw_flag = False;

/*	Superclass does all the work so far; checks for redisplay, etc.
*/

	return (redraw_flag);
}



/*-------------------------------------------------------------
**	Composite Procs
**-------------------------------------------------------------
*/

/*-------------------------------------------------------------
**	ChangeManaged
**		Handle change in set of managed children.
*/
static void 
#ifdef _NO_PROTO
ChangeManaged( manager )
        Widget manager ;
#else
ChangeManaged(
        Widget manager )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr 	= (DtDialogBoxWidget) manager;
	Dimension		w 	= M_Width (mgr),
				h 	= M_Height (mgr);
/*	Compute desired size.
*/
	M_GetSize ((Widget)mgr, 0, 0, 0, 0, &w, &h);

	/* check for resize policy if not first time ?? */

/*	Try to change size to fit children
*/
	if (w != M_Width (mgr) || h != M_Height (mgr))
	{
		switch (XtMakeResizeRequest ((Widget) mgr, w, h, &w, &h))
		{
			case XtGeometryAlmost:
				XtMakeResizeRequest ((Widget) mgr, w, h,
					NULL, NULL);
			case XtGeometryYes:
			case XtGeometryNo:
			default:
				break;
		}
	}
	
/*	Set positions and sizes of children.
*/
	M_Resize (manager);
}



/*-------------------------------------------------------------
**	InsertChild
**		Add a child.
*/
static void 
#ifdef _NO_PROTO
InsertChild( child )
        Widget child ;
#else
InsertChild(
        Widget child )
#endif /* _NO_PROTO */
{
	DtDialogBoxConstraint	constraint = (DtDialogBoxConstraint)
					M_DialogBoxConstraint (child);
	DtDialogBoxWidget	w = (DtDialogBoxWidget) XtParent (child);
	XmManagerWidgetClass	mc = (XmManagerWidgetClass)
						xmManagerWidgetClass;
	Dimension		s_t = M_ShadowThickness (w);
	Arg		al[20];		/*  arg list		*/
	register int	ac;		/*  arg count		*/

	(*mc->composite_class.insert_child) (child);

	if (constraint->child_type == XmWORK_AREA)
	{
		if (! M_WorkArea (w))
		{
			M_WorkArea (w) = child;
			ac = 0;
			XtSetArg (al[ac], XmNtopAttachment, XmATTACH_FORM);  ac++;
			XtSetArg (al[ac], XmNtopOffset, s_t);  ac++;
			XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM);  ac++;
			XtSetArg (al[ac], XmNleftOffset, s_t);  ac++;
			XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
			XtSetArg (al[ac], XmNrightOffset, s_t);  ac++;
			XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_WIDGET);  ac++;
			XtSetArg (al[ac], XmNbottomWidget, M_Separator (w));  ac++;
			XtSetArg (al[ac], XmNbottomOffset, 0);  ac++;
			XtSetValues (child, al, ac);
		}
	}
}



/*-------------------------------------------------------------
**	DeleteChild
**		Delete a child.
*/
static void 
#ifdef _NO_PROTO
DeleteChild( child )
        Widget child ;
#else
DeleteChild(
        Widget child )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	w = (DtDialogBoxWidget) XtParent (child);
	XmManagerWidgetClass	mc = (XmManagerWidgetClass)
						xmManagerWidgetClass;

	(*mc->composite_class.delete_child) (child);
	
	if (child == M_Separator (w))
		M_Separator (w) = NULL;
	else if (child == M_WorkArea (w))
		M_WorkArea (w) = NULL;

	/* button children ?? */
}


 
/*-------------------------------------------------------------
**	Constraint Procs
**-------------------------------------------------------------
*/
/*-------------------------------------------------------------
**	ConstraintInitialize
**		Add a child.
*/
static void 
#ifdef _NO_PROTO
ConstraintInitialize( request, new )
        Widget request ;
        Widget new ;
#else
ConstraintInitialize(
        Widget request,
        Widget new )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr	 = (DtDialogBoxWidget) XtParent (new);
	DtDialogBoxConstraint	constraint	= M_DialogBoxConstraint (new);

/*	Validate child type.
*/
	if (constraint->child_type != XmWORK_AREA &&
	    constraint->child_type != XmSEPARATOR &&
	    constraint->child_type != XmBUTTON)
	{
#ifdef NOTYET
		_XmWarning (new, WARN_CHILD_TYPE);
#endif
		if (! M_WorkArea (mgr))
		{
			constraint->child_type = XmWORK_AREA;
		}
	}
}


/*-------------------------------------------------------------
**	XmManager Procs
**-------------------------------------------------------------
*/

/*	All inherited from superclass.
*/



/*-------------------------------------------------------------
**	DtDialogBox Procs
**-------------------------------------------------------------
*/ 

/*-------------------------------------------------------------------------
**	GetSize
**		Calculate desired size based on children.
*/
static void 
#ifdef _NO_PROTO
GetSize( manager, wa_w, wa_h, bp_w, bp_h, w, h )
        Widget manager ;
        Dimension wa_w ;
        Dimension wa_h ;
        Dimension bp_w ;
        Dimension bp_h ;
        Dimension *w ;
        Dimension *h ;
#else
GetSize(
        Widget manager,
        Dimension wa_w,
        Dimension wa_h,
        Dimension bp_w,
        Dimension bp_h,
        Dimension *w,
        Dimension *h )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr =		(DtDialogBoxWidget) manager;
	Widget			work_area =	M_WorkArea (mgr),
				separator =	M_Separator (mgr);
	Widget *		button =	M_Button (mgr);
	register int		button_count =	M_ButtonCount (mgr),
				i;		      
	Boolean			min_btns =	M_MinimizeButtons (mgr);
	Dimension		sep_h =	0,
				offset = OFFSET, pad,
				max_w = 0, max_h = 0,
				s_t =		M_ShadowThickness (mgr);
	XtWidgetGeometry	request, reply;


/*	Query work area.
*/
	if (work_area && !wa_w && !wa_h)
	{
		request.request_mode = CWWidth | CWHeight;
		XtQueryGeometry (work_area, &request, &reply);
		wa_w = reply.width;
		wa_h = reply.height;
	}

	sep_h = (separator)	? P_Height (separator)	: 0;

/*	Get button panel size.
*/
	s_t = Max (2, s_t);
	if ((bp_w == 0) && (bp_h == 0) && !min_btns)
	{
		request.request_mode = CWWidth | CWHeight;
		for (i = 0;  i < button_count;  i++)
		{
			pad = (i == 0) ? 0 : 4 * s_t;
			XtQueryGeometry (button[i], &request, &reply);
			max_w = Max (max_w, reply.width + pad);
			max_h = Max (max_h, reply.height + pad);
		}

		if (button_count)
		{
			max_w += 4;
			max_h += 1;
		}

		bp_w = (button_count * max_w) + ((button_count + 1) * offset);
		bp_h = sep_h + max_h + (2 * offset);
	}

	else if ((bp_w == 0) && (bp_h == 0) && min_btns)
	{
		request.request_mode = CWWidth | CWHeight;
		for (i = 0;  i < button_count;  i++)
		{
			XtQueryGeometry (button[i], &request, &reply);
			bp_w += reply.width;
			max_h = Max (max_h, reply.height);
		}

		bp_w += (button_count + 1) * offset;
		bp_h = sep_h + max_h + (2 * offset);
	}

/*	Set width and height.
*/
	*w = Max (wa_w, bp_w);
	*h = wa_h + sep_h + bp_h;
}



/*-------------------------------------------------------------
**	CreateChildren
**		Create resource and value labels and text.
*/
static void 
#ifdef _NO_PROTO
CreateChildren( new, top_widget, bottom_widget )
        DtDialogBoxWidget new ;
        Widget top_widget ;
        Widget bottom_widget ;
#else
CreateChildren(
        DtDialogBoxWidget new,
        Widget top_widget,
        Widget bottom_widget )
#endif /* _NO_PROTO */
{
	Widget		ref_widget,
			sep, btn;
	Widget *	button = 	M_Button (new);
	int		count =		M_ButtonCount (new),
			i, l_p, r_p, off_p, b_delta, offset = 0;
	XmStringTable	string =	M_ButtonLabelStrings (new);
	Boolean		min_btns =	M_MinimizeButtons (new);
	Dimension	s_t = 		M_ShadowThickness (new);
	char		button_name[100];

	Arg		al[20];		/*  arg list		*/
	register int	ac;		/*  arg count		*/

/*	Compute position factors.
*/
	off_p = 2;
	b_delta = (100 - off_p) / count;
	l_p = (100 - (count * b_delta) + off_p) / 2;
	r_p = l_p + b_delta - off_p;
	s_t = Max (2, s_t);

/*	Create buttons.
*/
	for (i = 0; i < count; i++)
	{
		ac = 0;
		XtSetArg (al[ac], XmNdefaultButtonShadowThickness, s_t/2);
		ac++;
		XtSetArg (al[ac], XmNshadowThickness, s_t);  ac++;
		XtSetArg (al[ac], XmNhighlightThickness, s_t);  ac++;
		if (!min_btns)
		{
			XtSetArg (al[ac], XmNleftAttachment,
				XmATTACH_POSITION);  ac++;
			XtSetArg (al[ac], XmNleftPosition, l_p);  ac++;
			XtSetArg (al[ac], XmNleftOffset, offset);  ac++;
			XtSetArg (al[ac], XmNrightAttachment,
				XmATTACH_POSITION);  ac++;
			XtSetArg (al[ac], XmNrightPosition, r_p);  ac++;
			XtSetArg (al[ac], XmNrightOffset, offset);  ac++;
		}
		else
		{
			if (i == 0)
			{
				XtSetArg (al[ac], XmNleftAttachment,
					XmATTACH_FORM);  ac++;
				XtSetArg (al[ac], XmNleftOffset, OFFSET);  ac++;
			}
			else if (i == count -1)
			{
				XtSetArg (al[ac], XmNrightAttachment,
					XmATTACH_FORM);  ac++;
				XtSetArg (al[ac], XmNrightOffset, OFFSET);  ac++;
			}
			else
			{
				XtSetArg (al[ac], XmNleftAttachment,
					XmATTACH_WIDGET);  ac++;
				XtSetArg (al[ac], XmNleftWidget, button[i-1]);
				ac++;
				XtSetArg (al[ac], XmNleftOffset, OFFSET);
				ac++;
			}
		}

		XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
		XtSetArg (al[ac], XmNbottomOffset, OFFSET + offset);  ac++;
		if (string)
		{
			XtSetArg (al[ac], XmNlabelString, string[i]);  ac++;
		}
		XtSetArg (al[ac], XmNchildType, XmBUTTON);  ac++;
		sprintf (button_name, "%s_%d\0", M_Name (new), i+1);
		btn = XmCreatePushButtonGadget ((Widget)new, button_name, al, ac);
		XtManageChild (btn);
 		XtAddCallback (btn, XmNactivateCallback,
				(XtCallbackProc)ButtonCallback,
				(XtPointer)(i+1));

		button[i] = btn;
		l_p += b_delta;
		r_p += b_delta;

	}
	ref_widget = button[0];
	new->bulletin_board.default_button = button[0];

/*	Create Separator
*/
	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM);  ac++;
	XtSetArg (al[ac], XmNleftOffset, 0);  ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
	XtSetArg (al[ac], XmNrightOffset, 0);  ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_WIDGET);  ac++;
	XtSetArg (al[ac], XmNbottomWidget, ref_widget);  ac++;
	XtSetArg (al[ac], XmNbottomOffset, OFFSET);  ac++;
	XtSetArg (al[ac], XmNhighlightThickness, 0);  ac++;
	XtSetArg (al[ac], XmNchildType, XmSEPARATOR);  ac++;
	sep = XmCreateSeparatorGadget ((Widget)new, "separator", al, ac);
	XtManageChild (sep);
	M_Separator (new) = sep;
}


/*-------------------------------------------------------------
**	ButtonCallback
*/
static void 
#ifdef _NO_PROTO
ButtonCallback( g, client_data, call_data )
        Widget g ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
ButtonCallback(
        Widget g,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr = 	(DtDialogBoxWidget) XtParent (g);
	XtCallbackList		cb_list =	M_Callback (mgr);
	XmAnyCallbackStruct *	b_cb_data =
				(XmAnyCallbackStruct *) call_data;
	DtDialogBoxCallbackStruct	cb_data;

	if (cb_list != NULL)
	{
		cb_data.reason = XmCR_DIALOG_BUTTON;
		cb_data.event = b_cb_data->event;
		cb_data.button_position = (int) client_data;
		cb_data.button = g;
		XtCallCallbackList ((Widget) mgr, cb_list, &cb_data);
	}
}



/*-------------------------------------------------------------
**	Public Entry Points
**-------------------------------------------------------------
*/

/*-------------------------------------------------------------
**      DtCreateDialogBox
**		Create a new DtDialogBox instance.
**-------------------------------------------------------------
*/
Widget 
#ifdef _NO_PROTO
DtCreateDialogBox( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
DtCreateDialogBox(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
	return (XtCreateWidget (name, dtDialogBoxWidgetClass, 
			parent, arglist, argcount));
}


/*-------------------------------------------------------------
**      DtCreateDialogBoxDialog
**		Create a DialogShell and a new DialogBox instance;
**		return the DialogBox widget;
**-------------------------------------------------------------
*/
Widget 
#ifdef _NO_PROTO
DtCreateDialogBoxDialog( ds_p, name, db_args, db_n )
        Widget ds_p ;
        String name ;
        ArgList db_args ;
        Cardinal db_n ;
#else
DtCreateDialogBoxDialog(
        Widget ds_p,
        String name,
        ArgList db_args,
        Cardinal db_n )
#endif /* _NO_PROTO */
{
	Widget		ds;		/*  DialogShell		*/
	Arg		ds_args[10];	/*  arglist for shell	*/
	Cardinal	ds_n;		/*  argcount for shell	*/
	Widget		db;		/*  new sb widget	*/


	/*  create DialogShell parent
	*/
	ds_n = 0;
	XtSetArg (ds_args[ds_n], XmNallowShellResize, True);  ds_n++;
	ds = XtCreatePopupShell (name, xmDialogShellWidgetClass, 
		ds_p, ds_args, ds_n);

	/*  create DialogBox, free args, return 
	*/
	db = XtCreateWidget (name, dtDialogBoxWidgetClass, 
		ds, db_args, db_n);
#ifdef SKS
	/*  create DialogBox, free args, return 
	*/
	db = XtCreateWidget (name, dtDialogBoxWidgetClass, 
		ds_p, db_args, db_n);
#endif

	return (db);
}


/*-------------------------------------------------------------
**      DtDialogBoxGetButton
**		Return DialogBox button.
**-------------------------------------------------------------
*/
Widget 
#ifdef _NO_PROTO
DtDialogBoxGetButton( w, pos )
        Widget w ;
        Cardinal pos ;
#else
DtDialogBoxGetButton(
        Widget w,
        Cardinal pos )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr =	(DtDialogBoxWidget) w;
	int			index =	pos - 1;
	Widget *		button = M_Button (mgr);
	Widget			rtn_w = NULL;


	if (index >= M_ButtonCount (mgr))
#ifdef NOTYET
		_XmWarning ((Widget)mgr, WARN_BUTTON_CHILD);
#endif
		;
	else
		rtn_w = button[index];

	return (rtn_w);
}


/*-------------------------------------------------------------
**      DtDialogBoxGetWorkArea
**		Return DialogBox work area;
**-------------------------------------------------------------
*/
Widget 
#ifdef _NO_PROTO
DtDialogBoxGetWorkArea( w )
        Widget w ;
#else
DtDialogBoxGetWorkArea(
        Widget w )
#endif /* _NO_PROTO */
{
	DtDialogBoxWidget	mgr =	(DtDialogBoxWidget) w;
	Widget			rtn_w =	M_WorkArea (mgr);

	return (rtn_w);
}



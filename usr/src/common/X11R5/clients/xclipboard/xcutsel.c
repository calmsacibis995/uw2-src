/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xclipboard:xcutsel.c	1.1"
/*
 * $XConsortium: xcutsel.c,v 1.15 91/02/17 12:05:27 dave Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Ralph Swick, DEC/Project Athena
 */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/StdSel.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xfuncs.h>

static XrmOptionDescRec optionDesc[] = {
    {"-selection", "selection",	XrmoptionSepArg, NULL},
    {"-select",    "selection",	XrmoptionSepArg, NULL},
    {"-sel",	   "selection",	XrmoptionSepArg, NULL},
    {"-s",	   "selection",	XrmoptionSepArg, NULL},
    {"-cutbuffer", "cutBuffer",	XrmoptionSepArg, NULL},
};

typedef struct {
    String  selection_name;
    int	    buffer;
    Atom    selection;
    char*   value;
    int     length;
} OptionsRec;

OptionsRec options;

#define Offset(field) XtOffsetOf(OptionsRec, field)

static XtResource resources[] = {
    {"selection", "Selection", XtRString, sizeof(String),
       Offset(selection_name), XtRString, "PRIMARY"},
    {"cutBuffer", "CutBuffer", XtRInt, sizeof(int),
       Offset(buffer), XtRImmediate, (caddr_t)0},
};

#undef Offset

typedef struct {
    Widget button;
    Boolean is_on;
} ButtonState;

static ButtonState state;

Syntax(call)
	char *call;
{
    fprintf (stderr, "usage:  %s [-selection name] [-cutbuffer number]\n", 
	     call);
    exit (1);
}


static void StoreBuffer(w, client_data, selection, type, value, length, format)
    Widget w;
    XtPointer client_data;
    Atom *selection, *type;
    XtPointer value;
    unsigned long *length;
    int *format;
{

    if (*type == 0 || *type == XT_CONVERT_FAIL || *length == 0) {
	XBell( XtDisplay(w), 0 );
	return;
    }

    XStoreBuffer( XtDisplay(w), (char*)value, (int)(*length),
		  options.buffer );
   
    XtFree(value);
}


static Boolean ConvertSelection(w, selection, target,
				type, value, length, format)
    Widget w;
    Atom *selection, *target, *type;
    XtPointer *value;
    unsigned long *length;
    int *format;
{
    Display* d = XtDisplay(w);
    XSelectionRequestEvent* req =
	XtGetSelectionRequest(w, *selection, (XtRequestId)NULL);
	
    if (*target == XA_TARGETS(d)) {
	Atom* targetP;
	Atom* std_targets;
	unsigned long std_length;
	XmuConvertStandardSelection(w, req->time, selection, target, type,
				   (caddr_t*)&std_targets, &std_length, format);
	*value = XtMalloc(sizeof(Atom)*(std_length + 4));
	targetP = *(Atom**)value;
	*length = std_length + 4;
	*targetP++ = XA_STRING;
	*targetP++ = XA_TEXT(d);
	*targetP++ = XA_LENGTH(d);
	*targetP++ = XA_LIST_LENGTH(d);
/*
	*targetP++ = XA_CHARACTER_POSITION(d);
*/
	bcopy((char*)std_targets, (char*)targetP, sizeof(Atom)*std_length);
	XtFree((char*)std_targets);
	*type = XA_ATOM;
	*format = 32;
	return True;
    }
    if (*target == XA_STRING || *target == XA_TEXT(d)) {
	*type = XA_STRING;
	*value = options.value;
	*length = options.length;
	*format = 8;
	return True;
    }
    if (*target == XA_LIST_LENGTH(d)) {
	long *temp = (long *) XtMalloc (sizeof(long));
	*temp = 1L;
	*value = (caddr_t) temp;
	*type = XA_INTEGER;
	*length = 1;
	*format = 32;
	return True;
    }
    if (*target == XA_LENGTH(d)) {
	long *temp = (long *) XtMalloc (sizeof(long));
	*temp = options.length;
	*value = (caddr_t) temp;
	*type = XA_INTEGER;
	*length = 1;
	*format = 32;
	return True;
    }
#ifdef notdef
    if (*target == XA_CHARACTER_POSITION(d)) {
	long *temp = (long *) XtMalloc (2 * sizeof(long));
	temp[0] = ctx->text.s.left + 1;
	temp[1] = ctx->text.s.right;
	*value = (caddr_t) temp;
	*type = XA_SPAN(d);
	*length = 2;
	*format = 32;
	return True;
    }
#endif /* notdef */
    if (XmuConvertStandardSelection(w, req->time, selection, target, type,
				    (caddr_t *)value, length, format))
	return True;

    /* else */
    return False;
}


static void SetButton(state, on)
    ButtonState *state;
    Boolean on;
{
    if (state->is_on != on) {
	Arg args[2];
	Pixel fg, bg;
	XtSetArg( args[0], XtNforeground, &fg );
	XtSetArg( args[1], XtNbackground, &bg );
	XtGetValues( state->button, args, TWO );
	args[0].value = (XtArgVal)bg;
	args[1].value = (XtArgVal)fg;
	XtSetValues( state->button, args, TWO );
	state->is_on = on;
    }
}


static void LoseSelection(w, selection)
    Widget w;
    Atom *selection;
{
    XtFree( options.value );
    options.value = NULL;
    SetButton(&state, False);
}


/* ARGSUSED */
static void Quit(w, closure, callData)
    Widget w;
    XtPointer closure;		/* unused */
    XtPointer callData;		/* unused */
{
    XtCloseDisplay( XtDisplay(w) );
    exit(0);
}


/* ARGSUSED */
static void GetSelection(w, closure, callData)
    Widget w;
    XtPointer closure;		/* unused */
    XtPointer callData;		/* unused */
{
    XtGetSelectionValue(w, options.selection, XA_STRING,
			StoreBuffer, NULL,
			XtLastTimestampProcessed(XtDisplay(w)));
}


/* ARGSUSED */
static void GetBuffer(w, closure, callData)
    Widget w;
    XtPointer closure;
    XtPointer callData;		/* unused */
{
    XtFree( options.value );
    options.value =
	XFetchBuffer(XtDisplay(w), &options.length, options.buffer);
    if (options.value != NULL) {
	if (XtOwnSelection(w, options.selection,
			   XtLastTimestampProcessed(XtDisplay(w)),
			   ConvertSelection, LoseSelection, NULL))
	    SetButton((ButtonState*)closure, True);
    }
}


void main(argc, argv)
    int argc;
    char *argv[];
{
    char label[100];
    Widget box, button;
    XtAppContext appcon;
    Widget shell =
	XtAppInitialize( &appcon, "XCutsel", optionDesc, XtNumber(optionDesc),
			 &argc, argv, NULL, NULL, 0 );
    XrmDatabase rdb = XtDatabase(XtDisplay(shell));

    if (argc != 1) Syntax(argv[0]);

    XtGetApplicationResources( shell, (caddr_t)&options,
			       resources, XtNumber(resources),
			       NULL, ZERO );

    options.value = NULL;
    XmuInternStrings( XtDisplay(shell), &options.selection_name, ONE,
		      &options.selection );

    box = XtCreateManagedWidget("box", boxWidgetClass, shell, NULL, ZERO);

    button =
	XtCreateManagedWidget("quit", commandWidgetClass, box, NULL, ZERO);
	XtAddCallback( button, XtNcallback, Quit, NULL );

    /* %%% hack alert... */
    sprintf(label, "*label:copy %s to %d",
	    options.selection_name,
	    options.buffer);
    XrmPutLineResource( &rdb, label );

    button =
	XtCreateManagedWidget("sel-cut", commandWidgetClass, box, NULL, ZERO);
	XtAddCallback( button, XtNcallback, GetSelection, NULL );

    sprintf(label, "*label:copy %d to %s",
	    options.buffer,
	    options.selection_name);
    XrmPutLineResource( &rdb, label );

    button =
	XtCreateManagedWidget("cut-sel", commandWidgetClass, box, NULL, ZERO);
	XtAddCallback( button, XtNcallback, GetBuffer, (XtPointer)&state );
 	state.button = button;
	state.is_on = False;
   
    XtRealizeWidget(shell);
    XtAppMainLoop(appcon);
}

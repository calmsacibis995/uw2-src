/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:dogs/dogs.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: dogs.c,v $ $Revision: 1.5 $ $Date: 92/03/13 15:34:42 $"
#endif
#endif

/*****************************************************************************
*
*  dogs.c - Square & Dog widget demo source file.
*  
******************************************************************************/

#include <stdio.h>
#include <Xm/Xm.h>
#include <Mrm/MrmPublic.h>
#include <Square.h>
#include <Dog.h>

XtAppContext  app_context;

#define k_dog1_id 1
#define k_dog2_id 2
#define k_dog3_id 3
#define k_help_id 4

static void create_cb();
static void bark_cb();
static void tb_cb();
static void scale_cb();
static void help_cb();
static void exit_cb();

static MrmHierarchy mrm_id;
static char *mrm_vec[]={"dogs.uid"};
static MrmCode mrm_class;
static MRMRegisterArg mrm_names[] = {
        {"create_cb", (caddr_t)create_cb },
        {"bark_cb", (caddr_t)bark_cb },
        {"tb_cb", (caddr_t)tb_cb },
        {"scale_cb", (caddr_t)scale_cb },
        {"help_cb", (caddr_t)help_cb },
        {"exit_cb", (caddr_t)exit_cb }
};

static Widget dog1_id;
static Widget dog2_id;
static Widget dog3_id;
static Widget help_id;

main(argc, argv)
    int argc;
    char **argv;
{
    Widget shell;
    Display *display;
    Widget app_main = NULL;
    Arg args[3];

    MrmInitialize ();
    SquareMrmInitialize();
    DogMrmInitialize();

    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();
    display = XtOpenDisplay(app_context, NULL, argv[0], "Dogs",
			NULL, 0, &argc, argv);
    
    if (display == NULL) {
	    fprintf(stderr, "%s:  Can't open display\n", argv[0]);
	    exit(1);
    }

    XtSetArg (args[0], XtNallowShellResize, True);
    XtSetArg (args[1], XtNminWidth, 620);
    XtSetArg (args[2], XtNminHeight, 370);
    shell = XtAppCreateShell(argv[0], NULL, applicationShellWidgetClass,
			  display, args, 3);

    if (MrmOpenHierarchy (1, mrm_vec, NULL, &mrm_id) != MrmSUCCESS) exit(0);
    MrmRegisterNames(mrm_names, XtNumber(mrm_names));
    MrmFetchWidget (mrm_id, "app_main", shell, &app_main, &mrm_class);
    XtManageChild(app_main);
    XtRealizeWidget(shell);
    XtAppMainLoop(app_context);
}

static void create_cb(w, id, reason)
    Widget w;
    int *id;
    unsigned long *reason;
{
    switch (*id) {
        case k_dog1_id: dog1_id = w; break;
        case k_dog2_id: dog2_id = w; break;
        case k_dog3_id: dog3_id = w; break;
        case k_help_id:
	    help_id = w;
	    XtUnmanageChild((Widget)XmMessageBoxGetChild(help_id,
				XmDIALOG_CANCEL_BUTTON));
	    XtUnmanageChild((Widget)XmMessageBoxGetChild(help_id,
				XmDIALOG_HELP_BUTTON));
	    break;
    }
}

static void bark_cb (w, volume, cb)
    Widget w;
    int *volume;
    XtPointer cb;
{
    XBell(XtDisplay(w), *volume);
}

static void tb_cb (w, tag, cb)
    Widget w;
    int *tag;
    XmToggleButtonCallbackStruct *cb;
{
    Arg args[1];
    Widget dog;

    switch (*tag) {
	case(1) : dog = dog1_id; break;
	case(2) : dog = dog2_id; break;	
	case(3) : dog = dog3_id; break;	
    }
    XtSetArg(args[0], SquareNmakeSquare, cb->set);
    XtSetValues(dog, args, 1);
}

static void scale_cb(w, tag, cb)
    Widget w;
    int *tag;
    XmScaleCallbackStruct *cb;
{
    Arg args[1];
    Widget dog;

    switch (*tag) {
	case(1) : dog = dog1_id; break;
	case(2) : dog = dog2_id; break;	
	case(3) : dog = dog3_id; break;	
    }
    XtSetArg(args[0], DogNwagTime, cb->value);
    XtSetValues(dog, args, 1);
}

static void help_cb (w, name, cb)
    Widget w;
    XmString name;
    XtPointer cb;
{
    Arg args[1];

    if (name == NULL) return;
    XtSetArg (args[0], XmNmessageString, name);
    XtSetValues(help_id, args, 1);
    XtManageChild(help_id);
}

static void exit_cb (w, name, cb)
    Widget w;
    XmString name;
    XtPointer cb;
{
    exit(0);
}

